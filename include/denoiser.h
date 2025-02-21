/*
 * Sound denoiser
 */
#ifndef _DENOISER_H_
#define _DENOISER_H_

typedef struct denoiser {
	short*  SigBuf;
	long   Spd_N  [258];
	short  OutBuf [256];
	short  AddBuf [128];
	long   Snr    [258];
	long   Spd_X  [258];
	short  *Gain;

	short* Spd_I;
	short* fpArr;

	long filter_freq;
	long Gain_threshold;

	long Gain_sens_m;
	long Gain_sens_d;
	long Gain_nois_m;
	long Gain_nois_d;

	long  Rms_fast;
	long  Rms_aver;

	short count;
	short sampling_rate;
	short S_rate;
	short i_start;
	short i_fin;
	short  fft_order;
	short  fft_size;
	short *fft_sin;
	short *fft_sin_rev;
	long  *fft_ldata;
	struct {
		short supr_max;		// max suppresion dB
		short spec_smo;		// Numb.of spec_smoo point 0=1..3=7
		short spec_smo_num;	// real num of spec_smoo point = spec_smo*2+1
		short pict;		// Garmonics suppression
		short correct;		// No tembrocorrection filter
		short ampl;		// Spec Correct amplification dB
		short sensi;		// Sig Contrast
		short sensp;		// Supression_1
		short rate;		// Defaults suppression params
		short tsmoo;		// Numb.of time_smoo point 0=10..5=5
		short cor_k1;		// LF_Gain dB (Old)
		short cor_k2;		// HF_Gain=Fm_Gain dB
		short cor_f1;		// LF Hz
		short smooth_length;	// Smoothing time in 0.1 sec
		short cor_f2;		// HF Hz
	}  Madsubfi;
	struct {
		short first;
		short last;
	} frame;

	short accumulate_count;
	short method;
} denoiser_t;


void denoiser_init(denoiser_t *dn);

void denoiser_start(denoiser_t *dn, short *input_data, short input_data_dim, short *used);

void denoiser_process(denoiser_t *dn, short *input_data, short input_data_dim,
		short *used, short *output_data, short *output_data_dim);

void denoiser_set_sample_rate(denoiser_t *dn, long sampling_rate);

#endif

