#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <gtk/gtk.h>

#include "wwviaudio.h"

#define MAXFREQ 8000
#define MINFREQ 20

static volatile int mousex;
static volatile int mousey;
static int real_screen_width = 800;
static int real_screen_height = 600;
static int frame_rate_hz = 60;

struct sample_clip {
	float *s;
	int nsamples;
};

void free_sample(struct sample_clip *s)
{
	free(s->s);
	s->s = NULL;
	free(s);
}

float triangle(float angle)
{
	return angle / (M_PI * 2.0);
}

struct sample_clip *make_raw_sample(int nsamples, float freq)
{
	int i;
	struct sample_clip *s;
	float angle;

	s = malloc(sizeof(*s));
	s->s = malloc(sizeof(float) * nsamples);
	s->nsamples = nsamples; 

	for (i = 0; i < nsamples; i++) {
		angle = (float) i * freq / (float) nsamples / (2.0 * M_PI);
		s->s[i] = sinf(angle);
	}
	return s;
}

#define NOCTAVES 8
float frequency[12 * NOCTAVES];

float sin_envelope(int sample, int nsamples)
{
	return sinf(((float) sample / (float) nsamples) * M_PI);
}

struct sample_clip *make_freq_sample(struct sample_clip *input, int nsamples, float freq)
{
	int i, j, limit;

	struct sample_clip *s;

	s = malloc(sizeof(*s));
	s->s = malloc(sizeof(float) * nsamples);
	s->nsamples = nsamples;

	limit = 44100.0f / freq; 
	j = 0;
	for (i = 0; i < nsamples; i++) {
		s->s[i] = sin_envelope(i, nsamples) * input->s[j];
		j = (j + 1) % limit;
	}
	return s;
}

#define MAXVOICES 32 

struct voice {
	volatile float freq;
	volatile float target_freq;
	float phase;
	volatile int samples_per_period;
	float lastv;
	volatile float right, left;
} v[MAXVOICES];

static void init_voices(void)
{
	int i;

	for (i = 0; i < MAXVOICES; i++) {
		v[i].freq = (((float) rand() / (float) RAND_MAX) * (MAXFREQ - MINFREQ) + MINFREQ);
		v[i].target_freq = v[i].freq;
		v[i].phase = ((float) rand() / (float) RAND_MAX) * M_PI * 2.0;
		v[i].samples_per_period = 44100.0f / v[i].freq;
		v[i].left = 0.5;
		v[i].right = 0.5;
	}
}

void voice_sample(uint64_t time, unsigned long nframes, float *out)
{
	int i, j;
	float left_output, right_output;

	for (j = 0; j < nframes; j++) {
		left_output = 0.0f;
		right_output = 0.0f;
		for (i = 0; i < MAXVOICES; i++) {
			float t = fmodf((float) time + (float) j, (float) v[i].samples_per_period);

			t = (t / (float) v[i].samples_per_period) * M_PI * 2.0f;
			t = 0.1f * sinf(t + v[i].phase);
			/* smooth it out */
			if (fabs(t - v[i].lastv) > 0.02) {
				t = v[i].lastv + 0.02 * (t - v[i].lastv);
				v[i].lastv = t;
			}
			left_output += t * v[i].left;
			right_output += t * v[i].right;
		}
		*out++ = left_output; 
		*out++ = right_output;
	}
}

static int main_da_motion_notify(GtkWidget *w, GdkEventMotion *event,
        __attribute__((unused)) void *unused)
{
	int i;
	float r;
	float f;
	float dx, dy;
	int n, o, no;
	int interval;

	mousex = event->x;
	mousey = event->y;

	for (i = 0; i < MAXVOICES; i++) {
		dx = (float) mousex / (float) real_screen_width;
		dy = (float) mousey / (float) real_screen_height;
		r = ((float) rand() / (float) RAND_MAX) * MAXFREQ * 0.05;
		r = r * dx;
		//r = 0.0f;
		n = (int) (dy * 12.0f);
		//o = (int) (((float) rand() / (float) RAND_MAX) * NOCTAVES);
		o = i % (((int)((dx + dy) * (NOCTAVES - 1))) + 1);
		o = 1 << o;
		f = frequency[n] * o + r;
		interval = (int) (i + ((dx + dy) / 2.0f) * 6.0) % 6; 
		if (interval >= 2)
			f = f * (float) interval / (float) (interval - 1);
		v[i].target_freq = f;
		v[i].right = 0.5 * (sinf(i * (dx + dy) * M_PI * 2.0) + 1.0);
		v[i].left = 1.0 - v[i].right;
	}
	return 1;
}

gint advance_game(gpointer data)
{
	int i;

	for (i = 0; i < MAXVOICES; i++) {
		v[i].freq += 0.05 * (v[i].target_freq - v[i].freq);
		v[i].samples_per_period = 44100.0f / v[i].freq;
	}
	return 1;
}
			
int main(int argc, char *argv[])
{
	int i, o;
	float twelfth_root_of_two = 1.05946309436;
	float samples, f, of = 27.5f;
	struct sample_clip *k = make_raw_sample(5000, 440);
	GtkWidget *window, *vbox, *main_da;
	GdkGC *gc = NULL;               /* our graphics context. */
	gint timer_tag;

	if (wwviaudio_initialize_portaudio(MAXVOICES, MAXVOICES) != 0) {
		fprintf(stderr, "failed to init portaudion\n");
		exit(1);
	}

	init_voices();

	for (o = 0; o < NOCTAVES; o++) {
		f = of;
		for (i = 0; i < 12; i++) {
			samples = 44100.0f / f; 
			printf("f = %f, s = %f,  t = %f\n",
				f, samples, 1000.0f / f);
			frequency[o * 12 + i] = f;
			f = f * twelfth_root_of_two;
		}
		printf("-----\n");
		of *= 2;
	}

	gtk_set_locale();
	gtk_init(&argc, &argv);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_container_set_border_width(GTK_CONTAINER (window), 0);
        vbox = gtk_vbox_new(FALSE, 0);
        main_da = gtk_drawing_area_new();

	gtk_widget_add_events(main_da, GDK_POINTER_MOTION_MASK);
	g_signal_connect(G_OBJECT (main_da), "motion_notify_event",
		G_CALLBACK (main_da_motion_notify), NULL);

        gtk_container_add (GTK_CONTAINER (window), vbox);
        gtk_box_pack_start(GTK_BOX (vbox), main_da, TRUE /* expand */, TRUE /* fill */, 0);
        gtk_window_set_default_size(GTK_WINDOW(window), real_screen_width, real_screen_height);
        gtk_widget_show (vbox);
        gtk_widget_show (main_da);
        gtk_widget_show (window);

        GdkColor black = { 0, 0, 0, 0 };
        gtk_widget_modify_bg(main_da, GTK_STATE_NORMAL, &black);

	gc = gdk_gc_new(GTK_WIDGET(main_da)->window);
#ifndef GLIB_VERSION_2_32
	/* this is only needed in glibc versions before 2.32 */

	/* Apparently (some versions of?) portaudio calls g_thread_init(). */
	/* It may only be called once, and subsequent calls abort, so */
	/* only call it if the thread system is not already initialized. */
	if (!g_thread_supported ())
		g_thread_init(NULL);
#endif
	timer_tag = g_timeout_add(1000 / frame_rate_hz, advance_game, NULL);

	gdk_threads_init();
	gtk_main();
	sleep(5);

	wwviaudio_cancel_all_sounds();
	wwviaudio_stop_portaudio();

	return 0;
}


