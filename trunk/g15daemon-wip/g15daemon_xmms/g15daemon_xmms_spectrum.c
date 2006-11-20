/*
    This file is part of g15daemon.

    g15daemon is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g15daemon is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with g15daemon; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    
    (c) 2006 Mike Lampard, Philip Lawatsch, and others
    
    This daemon listens on localhost port 15550 for client connections,
    and arbitrates LCD display.  Allows for multiple simultaneous clients.
    Client screens can be cycled through by pressing the 'L1' key.

    simple analyser xmms plugin for g15daemon
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <g15daemon_client.h>
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <glib.h>
#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/xmmsctrl.h>
#include <xmms/configfile.h>
#include <libg15.h>
#include <libg15render.h>

#define NUM_BANDS 16        

static gint16 bar_heights[NUM_BANDS];
static gint16 scope_data[G15_LCD_WIDTH];

static void g15analyser_init(void);
static void g15analyser_cleanup(void);
static void g15analyser_playback_start(void);
static void g15analyser_playback_stop(void);
static void g15analyser_render_pcm(gint16 data[2][512]);
static void g15analyser_render_freq(gint16 data[2][256]);

g15canvas *canvas;
static unsigned int leaving=0;
static unsigned int vis_type=0;
static unsigned int playing=0, paused=0;
pthread_t g15send_thread_hd;
pthread_t g15keys_thread_hd;
static int g15screen_fd = -1;

pthread_mutex_t g15buf_mutex;

VisPlugin g15analyser_vp = {
    NULL,
    NULL,
    0,
    "G15daemon Spectrum Analyzer 0.3",
    1,
    1,
    g15analyser_init, /* init */
    g15analyser_cleanup, /* cleanup */
    NULL, /* about */
    NULL, /* configure */
    NULL, /* disable_plugin */
    g15analyser_playback_start, /* playback_start */
    g15analyser_playback_stop, /* playback_stop */
    g15analyser_render_pcm, /* render_pcm */
    g15analyser_render_freq  /* render_freq */
};


VisPlugin *get_vplugin_info(void) {
    return &g15analyser_vp;
}

void g15spectrum_read_config(void)
{
    ConfigFile *cfg;
    gchar *filename;

    vis_type = 0;

    filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
    cfg = xmms_cfg_open_file(filename);

    if (cfg)
    {
        xmms_cfg_read_int(cfg, "G15Daemon Spectrum", "visualisation_type", (int*)&vis_type);
        xmms_cfg_free(cfg);
    }
    g_free(filename);
}

void g15spectrum_write_config(void)
{
    ConfigFile *cfg;
    gchar *filename;

    filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
    cfg = xmms_cfg_open_file(filename);

    if (cfg)
    {
        xmms_cfg_write_int(cfg, "G15Daemon Spectrum", "visualisation_type", vis_type);
        xmms_cfg_write_file(cfg, filename);
        xmms_cfg_free(cfg);
    }
    g_free(filename);
}

void *g15keys_thread() {
    int keystate = 0;
    struct pollfd fds;

    fds.fd = g15screen_fd;
    fds.events = POLLIN;

    while(!leaving){
	if ((poll(&fds, 1, 5)) > 0);
	  read (g15screen_fd, &keystate, sizeof (keystate));
	if (keystate)
	  {
		pthread_mutex_lock (&g15buf_mutex);
		switch (keystate)
		  {
		  	case G15_KEY_L1:
			  vis_type = 1 - vis_type;
                          g15spectrum_write_config(); // save as default next time
			  break;
			case G15_KEY_L2:
			  {
			  	if (playing)
				  {
				  	if (paused)
					  {
					  	xmms_remote_play(0);
						paused = 0;
					  }
					else
					  {
					  	xmms_remote_pause(0);
						paused = 1;
					  }
				  }
				else
				  xmms_remote_play(0);
				break;
			  }
			case G15_KEY_L3:
			  {
			  	if (playing)
				  xmms_remote_stop(0);
				break;
			  }
			case G15_KEY_L4:
			  {
			  	if (playing)
				  xmms_remote_playlist_prev(0);
				break;
			  }
			case G15_KEY_L5:
			  {
			  	if (playing)
				  xmms_remote_playlist_next(0);
				break;
			  }
			default:
			  break;
		  }
		keystate = 0;
		pthread_mutex_unlock (&g15buf_mutex);
	  }
        xmms_usleep(25000);
    }
    return NULL;
}

void *g15send_thread() {
    int i;
    int playlist_pos;
    char *title;
    char *artist;
    char *song;
    char *strtok_ptr;

    while(!leaving){
        pthread_mutex_lock (&g15buf_mutex);
	g15r_clearScreen (canvas, G15_COLOR_WHITE);

	if (xmms_remote_get_playlist_length(0) > 0)
	  {
	        playlist_pos = xmms_remote_get_playlist_pos(0);
		
	        title = xmms_remote_get_playlist_title(0, playlist_pos);
	        if(title!=NULL){ //amarok doesnt support providing title info via xmms interface :(
	            if(strlen(title)>32) {
	                artist = strtok_r(title,"-",&strtok_ptr);
	                song = strtok_r(NULL,"-",&strtok_ptr);
	                if(strlen(song)>32)
	                    song[32]='\0';
			g15r_renderString (canvas, (unsigned char *)song+1, 0, G15_TEXT_MED, 165-(strlen(song)*5), 0);
	                if(strlen(artist)>32)
	                    artist[32]='\0';
	                if(artist[strlen(artist)-1]==' ')
	                    artist[strlen(artist)-1]='\0';
	                g15r_renderString (canvas, (unsigned char *)artist, 0, G15_TEXT_MED, 160-(strlen(artist)*5), 8);
	            } else
			g15r_renderString (canvas, (unsigned char *)title, 0, G15_TEXT_MED, 160-(strlen(title)*5), 0);
	        }
	        g15r_drawBar (canvas, 0, 39, 159, 41, G15_COLOR_BLACK, xmms_remote_get_output_time(0)/1000, xmms_remote_get_playlist_time(0,playlist_pos)/1000, 1);

		if (playing)
		  {
			if (vis_type == 0)
			  {
			        for(i = 0; i < NUM_BANDS; i++)
			          {               
				    int y1 = (40 - bar_heights[i]);
				    if (y1 > 36)
				      continue;
				    g15r_pixelBox (canvas, (i * 10), y1, ((i * 10) + 8), 36, G15_COLOR_BLACK, 1, 1);
			          }
			  }
			else
			  {
				int y1, y2=(25 - scope_data[0]);
				for (i = 0; i < 160; i++)
				  {
				    y1 = y2;
				    y2 = (25 - scope_data[i]);
				    g15r_drawLine (canvas, i, y1, i + 1, y2, G15_COLOR_BLACK);
				  }
			  }
		  }
		else 
		  g15r_renderString (canvas, (unsigned char *)"Playback Stopped", 0, G15_TEXT_LARGE, 16, 16);
	  }
	else
	  g15r_renderString (canvas, (unsigned char *)"Playlist Empty", 0, G15_TEXT_LARGE, 24, 16);

        if(g15_send(g15screen_fd,(char *)canvas->buffer,G15_BUFFER_LEN)<0) {
             /* connection error occurred - try to reconnect to the daemon */
            while((g15screen_fd=new_g15_screen(G15_G15RBUF))<0 && !leaving){
              xmms_usleep(150000);
            }
        }
        pthread_mutex_unlock(&g15buf_mutex);
        xmms_usleep(25000);
    }
    return NULL;
}


static void g15analyser_init(void) {

    pthread_mutex_init(&g15buf_mutex, NULL);        
    pthread_mutex_lock(&g15buf_mutex);
    
    if((g15screen_fd = new_g15_screen(G15_G15RBUF))<0){
        return;
    }
    canvas = (g15canvas *) malloc (sizeof (g15canvas));
    if (canvas != NULL)
      {
      	memset(canvas->buffer, 0, G15_BUFFER_LEN);
	canvas->mode_cache = 0;
	canvas->mode_reverse = 0;
	canvas->mode_xor = 0;
      }

    leaving = 0;
    g15spectrum_read_config();
    pthread_mutex_unlock(&g15buf_mutex);
    /* increase lcd drive voltage/contrast for this client */
    g15_send_cmd(g15screen_fd, G15DAEMON_CONTRAST,2);

    pthread_attr_t attr;
    memset(&attr,0,sizeof(pthread_attr_t));
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create(&g15send_thread_hd, &attr, g15send_thread, 0);
    pthread_create(&g15keys_thread_hd, &attr, g15keys_thread, 0);
 }

static void g15analyser_cleanup(void) {
    
    pthread_mutex_lock (&g15buf_mutex);
    leaving=1;
    pthread_mutex_unlock (&g15buf_mutex);
    pthread_join (g15send_thread_hd, NULL);
    pthread_join (g15keys_thread_hd, NULL);
    
    pthread_mutex_lock (&g15buf_mutex);
    if (canvas != NULL)
      free(canvas);
    if(g15screen_fd)
      close(g15screen_fd);
    pthread_mutex_unlock (&g15buf_mutex);
    return;
}

static void g15analyser_playback_start(void) {
    
    pthread_mutex_lock (&g15buf_mutex);
    playing = 1;
    paused = 0;
    pthread_mutex_unlock (&g15buf_mutex);
    return;

}

static void g15analyser_playback_stop(void) {

    pthread_mutex_lock (&g15buf_mutex);
    playing = 0;
    paused = 0;
    pthread_mutex_unlock (&g15buf_mutex);
    return;

}

static void g15analyser_render_pcm(gint16 data[2][512]) {
    pthread_mutex_lock (&g15buf_mutex);

    if (playing && !leaving)
      {
	    gint i;
	    gint max;
	    gint scale = 128;
	
	    do
	      {
		    max = 0;
		    for (i = 0; i < G15_LCD_WIDTH; i++)
		      {
			scope_data[i] = data[0][i] / scale;
			if (abs(scope_data[i]) > abs(max))
			  max = scope_data[i];
		      }
		    scale += 128;
	      } while (abs(max) > 10);
      }

    pthread_mutex_unlock (&g15buf_mutex);
}

static void g15analyser_render_freq(gint16 data[2][256]) {
    
    pthread_mutex_lock(&g15buf_mutex);

    if (playing && !leaving)
      {
	    gint i;
	    gdouble y;
	    int j;
	    int xscale[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74,101, 137, 187, 255};
	
	    if(!g15screen_fd)
	        return;
	    
	    for(i = 0; i < NUM_BANDS; i++)
	    {
	      for(j=xscale[i], y=0; j < xscale[i+1]; j++)
	      {
		  if(data[0][j] > y)
		    y = data[0][j];
	      }
	
	      if(y)
	      {         
	          y = (gint)(log(y) * (14/log(64)));
	          if(y > 32) y = 32;
	      }
	
	      bar_heights[i] = y;
	    }
      }

    pthread_mutex_unlock(&g15buf_mutex);

    return;
}

