/*
 * SDL Sound Driver
 *
 * Copyright (C) 2007, Joseph Jezak (josejx@gentoo.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License, version 2
 * as published by the Free Software Foundation
 *
 * This isn't fancy, it's basically just a shim between the MOL audio driver
 * on the host and the SDL Audio library.  No mixer, no complications. :)
 *
 */

#include <SDL/SDL.h>
#include <SDL/SDL_audio.h>

#include "mol_config.h"
#include "sound-iface.h"

#define NUM_SDL_SOUND_BUFFERS	2

/* This struct holds persistant data for the sdl sound device */
typedef struct {
	/* Left and right channel volume */
	int lvol;
	int rvol;

	/* Current sound format (AudioSpec) */
	SDL_AudioSpec *format;

	/* Sound buffer */
	unsigned char *buf;
	int buf_sz;
	/* Length of sound in buffer */
	int buf_len;
	/* Audio Semaphore for access to the data */
	SDL_mutex *buf_lock;


} sdl_sound_info_t;

sdl_sound_info_t	sdl_snd;

/******************************************************************************
 * SDL Interface
 *****************************************************************************/

/* Callback function for SDL sound */
static void sdl_sound_stream (void *arg, u8 *stream, int stream_len) {
	SDL_LockMutex(sdl_snd.buf_lock);
	/* If we've got data to push */
	if (sdl_snd.buf_len > stream_len) {
		/* Copy the sound data to the stream */	
		memcpy(stream, sdl_snd.buf, stream_len);
		sdl_snd.buf_len = sdl_snd.buf_len - stream_len;
		SDL_UnlockMutex(sdl_snd.buf_lock);
	}
	/* If we don't have enough data to fill the stream */
	else if (sdl_snd.buf_len) {
		/* Copy the sound data to the stream */	
		memcpy(stream, sdl_snd.buf, sdl_snd.buf_len);
		/* Zero out any additional samples */
		memset(stream + sdl_snd.buf_len, 0, stream_len - sdl_snd.buf_len);
		sdl_snd.buf_len = 0;
		SDL_UnlockMutex(sdl_snd.buf_lock);
	}
	/* Otherwise, just write out silence */
	else {
		SDL_UnlockMutex(sdl_snd.buf_lock);
		memset(stream, 0, stream_len);
	}
}

/******************************************************************************
 * MOL Interface
 *****************************************************************************/

/* Initialize the AudioSpec struct */
static SDL_AudioSpec * sdl_sound_init_spec(int format, int rate) {
	SDL_AudioSpec *spec;
	spec = malloc(sizeof(SDL_AudioSpec));
	if (spec == NULL)
		return NULL;
	memset(spec, 0, sizeof(SDL_AudioSpec));
	
	/* Find the desired rate */
	spec->freq = rate;
	/* Find format */
	switch(format & kSoundFormatMask) {
	case kSoundFormat_S16_BE:
		spec->format = AUDIO_S16MSB;	
		break;
	case kSoundFormat_S16_LE:
		spec->format = AUDIO_S16LSB;	
		break;
	/* Default to U8 - Should always work... */
	case kSoundFormat_U8:
	default:
		spec->format = AUDIO_U8;	
		break;
	}
	
	/* If stereo is enabled, use two channels */
	if(format & kSoundFormat_Stereo)
		spec->channels = 2;
	else
		spec->channels = 1;
	
	/* And a few defaults */
	spec->callback = sdl_sound_stream;
	spec->userdata = NULL;
	return spec;
}

/* Destroy audio device */
static void sdl_sound_cleanup (void) {
	return;
}

/* Query the audio device 
 *
 * Takes:
 * format - Sound Format Type (BE, etc.)
 * rate - Sound Rate (44KHz, 22kHz etc)
 * fragsize - Write the fragment size back to this var
 *
 * Returns:
 * -1 if we can't match format/rate
 * 0 if the rate is supported
 * 1 if it may be supported
 */
static int sdl_sound_query (int format, int rate, int *fragsize) {
	SDL_AudioSpec *s = sdl_sound_init_spec(format, rate);
	int ret = 0;
	int fmt = 0;

	/* Check if we got the Spec */
	if (s == NULL)
		return -1;
	
	/* Open the audio device */
	if (SDL_OpenAudio(s, s) < 0) {
		printm("Unable to open the audio device: %s", SDL_GetError());
		ret = -1;
	}
	else {
		/* Is the rate any good ? */
		if (s->freq != rate) 
			ret = -1;
		/* Is the format any good ? */
		fmt = format & kSoundFormatMask;	
		if (fmt == kSoundFormat_S16_BE && s->format != AUDIO_S16MSB)
			ret = -1;
		if (fmt == kSoundFormat_S16_LE && s->format != AUDIO_S16LSB)
			ret = -1;
		if (fmt == kSoundFormat_U8 && s->format != AUDIO_U8)
			ret = -1;
		*fragsize = s->samples;
		SDL_CloseAudio();
	}
	/* Free the spec */
	free(s);
	return ret;
}

/* Open the audio device */
static int sdl_sound_open (int format, int rate, int *fragsize, int ringmode) {
	printm("SDL Sound Open\n");
	sdl_snd.format = sdl_sound_init_spec(format, rate);
	if (sdl_snd.format == NULL) {
		printm("Unable to alloc the audio spec\n");
		return -1;
	}
	
	/* Open the audio device -- we assume that the format is okay from the query earlier */
	if (SDL_OpenAudio(sdl_snd.format, sdl_snd.format) < 0) {
		printm("Unable to open the audio device: %s", SDL_GetError());
		return -1;
	}
	*fragsize = sdl_snd.format->samples;

	/* Allocate memory for the buffer */
	sdl_snd.buf_sz = sdl_snd.format->samples * NUM_SDL_SOUND_BUFFERS;
	sdl_snd.buf = malloc(sdl_snd.buf_sz);
	sdl_snd.buf_len = 0;

	/* Okay, we're ready to start sound playback */
	SDL_PauseAudio(0);
	return 0;
}

/* Close audio device */
static void sdl_sound_close (void) {
	/* Cleanup ! */
	SDL_CloseAudio();
	/* Free the buffer */
	free(sdl_snd.buf);
	/* Free AudioSpec */
	if(sdl_snd.format != NULL)
		free(sdl_snd.format);
}
	
/* Write audio buffers */
static void sdl_sound_write (char *fragptr, int size) {
	SDL_LockMutex(sdl_snd.buf_lock);
	if(size <= sdl_snd.buf_sz) {
		/* Mix the incoming audio stream with the current volume */
		/* FIXME Forced to lvol for now */
		SDL_MixAudio(sdl_snd.buf, (unsigned char *) fragptr, size, sdl_snd.lvol);
		sdl_snd.buf_len = size;
		SDL_UnlockMutex(sdl_snd.buf_lock);
	}
	else {
		SDL_UnlockMutex(sdl_snd.buf_lock);
		/* Overrun! */
		printm("SDL_sound: Buffer overrun: Size: %i Buffer Length: %i Max Buffer Length: %i\n", size, sdl_snd.buf_len, sdl_snd.buf_sz);
	}
}

/* Flush the audio buffers */
static void sdl_sound_flush (void) {
	return;
}
	
/* Change the volume */
static void sdl_sound_volume (int lvol, int rvol) {
	/* Volumes should already be adjusted... */
	sdl_snd.lvol = lvol;
	sdl_snd.rvol = rvol;
}

static sound_ops_t sdl_sound_driver_ops = {
	.cleanup	= sdl_sound_cleanup,
	.query		= sdl_sound_query,
	.open		= sdl_sound_open,
	.close 		= sdl_sound_close,
	.write		= sdl_sound_write,
	.flush		= sdl_sound_flush,
	.volume		= sdl_sound_volume,
};

/* Exact indicates whether we called this explicitly or the user specified "any" */
sound_ops_t * sdl_sound_probe(int exact) {
	printm("SDL sound driver\n");
	/* Do we always say okay?  I think so... */
	sdl_snd.format = NULL;
	return &sdl_sound_driver_ops;
}
