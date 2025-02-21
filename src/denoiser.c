/*
 *
 */

#include <string.h>
#include <stdlib.h>

#include "fft.h"
#include "math_dsp.h"
#include "denoiser.h"

#define TEMP_CORRECT_NORM_RATE		10
#define TEMP_CORRECT_NORM		(1<<TEMP_CORRECT_NORM_RATE)

#define FILTER_NORM_RATE		12
#define FILTER_NORM			(1 << FILTER_NORM_RATE)

#define FREQ_HUMAN_MAX			760 // max of human ... sence freq in Hz
#define C_MAX				(1 << 11)

#define SHORT_MIN			0xFFFF8000L
#define B_MIN				1

#define SUPPRESS_0_DB			1024
#define SUPPRESS_6_DB			512
#define SUPPRESS_12_DB			256
#define SUPPRESS_18_DB			128
#define SUPPRESS_24_DB			64
#define SUPPRESS_30_DB			32

#define SUPPRESS_MAX			5
#define SUPPRESS_MIN			0


/* Adjusting the suppression depth */
static const short fp_arr_min[] = {
	SUPPRESS_0_DB,		//   0 dB
	SUPPRESS_6_DB,		//  +6 dB
	SUPPRESS_12_DB,		// +12 dB
	SUPPRESS_18_DB,		// +18 dB
	SUPPRESS_24_DB,		// +24 dB
	SUPPRESS_30_DB,		// +30 dB
};

inline void add_buff(short *dest, short *add, unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++)
		dest[i] += add[i];
}

inline void norm_sigmax(short *buf, short sig_max , short shift, unsigned int size)
{
	unsigned int i;

	for (i = 0; i < size; i++)
		buf[i] = ((long)buf[i] * sig_max) >> shift;
}

/*
 * Initialize denoiser functions
 */
void denoiser_init(denoiser_t * dn)
{
	memset(dn, 0, sizeof(denoiser_t));

	dn->Madsubfi.supr_max                =    0;
	dn->Madsubfi.spec_smo                =    4;
	dn->Madsubfi.spec_smo_num            =    0;
	dn->Madsubfi.pict                    =    0;
	dn->Madsubfi.ampl                    =    0;
	dn->Madsubfi.sensi                   =   40;
	dn->Madsubfi.sensp                   =   18;
	dn->Madsubfi.rate                    =    4;
	dn->Madsubfi.tsmoo                   =    4;
	dn->Madsubfi.correct                 =    1;
	dn->Madsubfi.cor_k1                  =    0;
	dn->Madsubfi.cor_k2                  =    0;

	dn->Madsubfi.cor_f1                  =  200;
	dn->Madsubfi.smooth_length           =   24; // 2.4 sec
	dn->Madsubfi.cor_f2                  = 3600;
}

/*
 *
 */
void denoiser_set_sample_rate(denoiser_t *dn, long sampling_rate)
{
	short fft_order;

	//  if (sampling_rate < 30000L)
	fft_order = 8;
	//  else
	//   fft_order = 10;

	dn->sampling_rate = sampling_rate;
	dn->filter_freq   = sampling_rate / 2;
	dn->fft_order     = fft_order;
	dn->fft_size      = 1 << fft_order;
}

/*
 * Startup denoiser
 */
void denoiser_start(denoiser_t *dn, short *input_data, 
		short input_data_dim, short *used)
{
	// Private variable declaration
	long g_left  = -30;
	long g_right = -80;
	long k1;
	long k2;
	short i, N;
	long f_step, k; 
	long Gain_long;

	N = dn->fft_size;
	dn->Gain_sens_m = (long)5 * dn->Madsubfi.sensi;
	dn->Gain_sens_d = (long)100;

	dn->Gain_nois_m = (long)205 * dn->Madsubfi.sensi * (long)dn->Madsubfi.sensp +
		(long)5000 * dn->Madsubfi.sensi - (long)200000;
	dn->Gain_nois_d = (long)100000;

	dn->S_rate     = (long)5 + dn->Madsubfi.tsmoo;
	dn->Gain_threshold = (long)4;

	// Manual make tembro-correction filter
	f_step = dn->filter_freq / N + 1;

	if (dn->Madsubfi.correct) {
		// Norming
		k1 = dn->Madsubfi.cor_k1 * TEMP_CORRECT_NORM;
		k2 = dn->Madsubfi.cor_k2 * TEMP_CORRECT_NORM;
		g_left  *= TEMP_CORRECT_NORM;
		g_right *= TEMP_CORRECT_NORM;
		//-------- (0 Hz,K1-30 dB)..(F1 Hz,K1 dB) -----------
		dn->i_start = 0;
		dn->i_fin = dn->Madsubfi.cor_f1 / f_step;
		k = (k1 - g_left) / dn->i_fin;
		for (i=dn->i_start; i<=dn->i_fin; i++) {
			Gain_long = k * i + g_left;

			if(Gain_long<(long)SHORT_MIN)
				dn->Gain [i] = (short)SHORT_MIN;
			else
				dn->Gain [i] = (short)Gain_long;
		}


		//-------- (F1 Hz,K1 dB)..(Fmax Hz,0 dB) -------------
		dn->i_start = dn->i_fin + 1;
		dn->i_fin = FREQ_HUMAN_MAX / f_step;
		k = - k1 / (dn->i_fin - dn->i_start);
		for (i=dn->i_start; i<=dn->i_fin; i++) 
			dn->Gain [i]= k * i + k1 * (1 + dn->i_start / (dn->i_fin - dn->i_start));

		//-------- (Fmax Hz,K1 dB)..(F2 Hz,K2 dB) ------------
		dn->i_start = dn->i_fin + 1; 
		dn->i_fin = dn->Madsubfi.cor_f2 / f_step;
		k = k2 / (dn->i_fin - dn->i_start);
		for (i=dn->i_start; i<=dn->i_fin; i++) 
			dn->Gain [i]= k * i - k2 * dn->i_start / (dn->i_fin - dn->i_start);

		//-------- (F2 Hz,K2 dB)..(2F2 Hz,-80 (was -50) dB) ---02.99
		dn->i_start = dn->i_fin + 1;
		dn->i_fin = N;
		k = - (k2 - g_right) / (dn->i_fin - dn->i_start);

		for (i=dn->i_start; i<=dn->i_fin; i++) {
			Gain_long = k * i + k2 * (1 + dn->i_start / (dn->i_fin - dn->i_start)) -
				g_right * dn->i_start / (dn->i_fin - dn->i_start);

			if(Gain_long<(long)SHORT_MIN)
				dn->Gain [i] = (short)SHORT_MIN;
			else
				dn->Gain [i] = (short)Gain_long;
		}

		//--------- dB SCALE -> float SCALE: 0..N ------------
		for (i=0; i<=N; i++)
			if (dn->Gain [i] >= 0)
				dn->Gain [i] = TEMP_CORRECT_NORM *
					(1 << (  dn->Gain [i] / TEMP_CORRECT_NORM / 6));
			else
				dn->Gain [i] = TEMP_CORRECT_NORM /
					(1 << (- dn->Gain [i] / TEMP_CORRECT_NORM / 6));

	} else {
		for (i = 0; i <= N; i++) 
			dn->Gain[i] = TEMP_CORRECT_NORM;
	}

	//----------- for voice detection ------------
	dn->i_start = dn->Madsubfi.cor_f1 / f_step;
	dn->i_fin   = dn->Madsubfi.cor_f2 / f_step;

	//--------------------------------------------
	dn->frame.first = 1;
	dn->frame.last  = 0;
	denoiser_process(dn, input_data, input_data_dim, used, 0, 0);
}

/*
 * Itself denoiser process
 */
static short max_arr(short *src, short n)
{
	short  max = 0;
	short i; 

	for (i = 0; i < n; i++)
		if (max < abs (src [i]))
			max = abs (src [i]);
	return max;
}

/*
 *--- 7-STEP OF GAIN FITTING: tembrocorrection -------------
 */
static void clear_noice_step_7(denoiser_t *dn)
{
	int i;

	if (!dn->Madsubfi.correct)
		return;

	for (i = 0; i <= dn->fft_size; i++) 
		dn->fpArr[i] = ((long)dn->fpArr[i] * dn->Gain[i]) >> TEMP_CORRECT_NORM_RATE;
}

/*
 *
 */
void denoiser_process(denoiser_t *dn, short *input_data,
		short input_data_dim, short *used,
		short *output_data, short *output_data_dim)
{
	short i,j;
	short N, N2, Ndiv2;
	short count_max;
	short Adap_rate;
	short SigMax;
	short fpArr_Norm;
	short p_fact;
	short SigMaxI;
	short shift,shift2;
	short fpArr_Min;
	short tr_1;
	long  x, s, sum,tmpl, tmp;
	long  g;

	short kj, mj;
	short s_rate_d;
	short s_rate_m;
	short Adap_rate_1, Adap_rate_2;
	short rate;
	long  voice_threshold;
	long  Q, Q_i;
	short norm;

	fpArr_Norm = 1 << 10 ;
	//  fpArr_Min  =  fpArr_Norm /gain_min [17];

	fpArr_Min = fp_arr_min[dn->Madsubfi.supr_max];

	// fpArr_Norm / gain_min [dn->Madsubfi.supr_max];

	N = dn->fft_size; 
	N2 = N * 2; 
	Ndiv2 = N / 2;

	for (i = 0; i < N; i++)
		dn->SigBuf[i+N] = 0;

	if (output_data_dim) (*output_data_dim) = 0;

	if (!dn->frame.last)
		for (i = dn->accumulate_count; i < N; i++)
			dn->SigBuf [i] = input_data [i - dn->accumulate_count];


	/* 2 ms*/
	if (dn->frame.first) {
		dn->Rms_aver = 0;
		for (i = 0; i <= N; i ++) {
			dn->Spd_I [i] = 0;
			dn->Spd_N [i] = B_MIN;
			dn->Snr   [i] = B_MIN;
			dn->Spd_X [i] = 0;
			dn->fpArr [i] = 0;
		}
	}
	/*2 ms*/
	//--- Adaptation rate =f( Filter parametres) ---------
	x = dn->filter_freq * dn->Madsubfi.smooth_length / 10 / N;
	if (x < 1) x = 1; 

	if (x > C_MAX)
		count_max = C_MAX;  
	else
		count_max = x; // max smooth time

	if (dn->count < count_max)
		dn->count += 1; 
	else
		dn->count = count_max;

	/*2 ms*/

	Adap_rate   = FILTER_NORM / dn->count;


	if (0 == dn->method) {
		Adap_rate_1 = Adap_rate * 12 / 100; // dn->Spd_N var rate in speech
		Adap_rate_2 = Adap_rate *  4 /  10; // N-Crit var rate in speech
	}

	// --- 1-STEP OF DATA PROCESS: FFT forward ------
	SigMax  = max_arr (dn->SigBuf, N);
	shift   = max_bit_for_forward (dn->fft_order + 1);

	/* 3 ms */
	if (SigMax) {
		for (i = 0; i < N; i++)
			dn->SigBuf[i] = ((long)dn->SigBuf[i]<<shift)/SigMax;
	}

	fft_forw(dn->SigBuf, dn->fft_ldata, dn->fft_sin, dn->fft_order + 1);

	if (0 == dn->method) {
		// Spd_I = sqrt (Re ** 2 + Im ** 2)
		for (i=1, j=N2-1; i<N; i++, j--) {
			tmpl = (long)dn->SigBuf[i] * dn->SigBuf[i];
			tmpl += (long)dn->SigBuf[j] * dn->SigBuf[j];
			dn->Spd_I[i] = (short)(((long)i_sqrt(tmpl)*SigMax)>>shift);
		} 

		//---------------- 0,N ----------------
		dn->Spd_I[0] = (short)((((long)abs(dn->SigBuf[0])*SigMax))>>shift);
		dn->Spd_I[N] = (short)((((long)abs(dn->SigBuf[N])*SigMax))>>shift);

		Q = 0;             

		//--- Pauses detection voice_threshold > 0 = voice, < 0 = noise --- 02.99
		for (i=dn->i_start; i<=dn->i_fin; i++) {
			Q_i = ((long)(dn->Spd_I[i]) << 2) / 10 - ((dn->Snr[i] + 
						dn->Snr[i] * 25 / 1000)>>12);

			if (Q_i <= 0)
				Q_i = 0; 
			else
				Q_i = (Q_i<<12) / (Q_i + (dn->Snr[i]>>12));

			Q += Q_i;
		}

		//------------------ criteria ----------------------
		rate = (N<<12) / dn->filter_freq;

		//--------------- Aver fitting ---------------------
		s = dn->Rms_aver;
		x = Q - dn->Rms_aver;

		//--- Noise: Middle up fast down -------------------
		if (x > 0)
			dn->Rms_aver += (((rate * x)>>12) / 10);
		else
			dn->Rms_aver += ((rate * x)>>12);

		dn->Rms_fast = Q; // Aver of deflections

		//========= voice_threshold: voice>0, noise<0 ===========
		voice_threshold = 4 * dn->Rms_fast / 10 - dn->Rms_aver;
		//===================================================

		if (dn->count < 4 && dn->count <= count_max) {
			voice_threshold  = -1; 
			dn->Rms_aver = dn->Rms_fast;
		} // begin steps = Noise

		//--- Speech: Slow up fast down ------------
		if (voice_threshold > 0) {
			if (x > 0)
				dn->Rms_aver = s + (((rate * x)>>11)/100);
			else
				dn->Rms_aver = s + ((rate * x)>>12);
		}

		//--- Noise spectr dn->Spd_N fitt in pauses ---02.99
		if (voice_threshold < 0)
			x = Adap_rate; 
		else
			x = Adap_rate_2;

		for (i=0; i<=N; i++) {
			s = (long)(dn->Spd_I[i]) - (dn->Snr[i]>>12);
			dn->Snr[i] += x * s;

			if (dn->Snr[i] < B_MIN) 
				dn->Snr[i] = B_MIN;
		}

		//----- SPEC SMOO IN FREQ DOMAIN -----
		if (dn->Madsubfi.spec_smo > 0) {
			for (i = 0; i <= N; i++) {
				sum = dn->Spd_I[i];

				for (j = 1; j <= dn->Madsubfi.spec_smo; j++) {
					mj = abs (i - j);
					kj = i + j;
					if (kj > N)
						kj = N2 - kj;

					sum += (long)dn->Spd_I[kj] + dn->Spd_I[mj];
				}

				sum -= (((long)dn->Spd_I[mj] + dn->Spd_I[kj])>>1); // 02.99 trapecia
				dn->fpArr[i] = sum>>3;
			}
			for (i = 0; i <= N; i++) 
				dn->Spd_I[i] = dn->fpArr[i];
		}

		//--- We use voice criteria for dn->Spd_N fitting in pauses ---
		if (voice_threshold < 0)
			x = Adap_rate; 
		else
			x = Adap_rate_1;

		for (i = 0; i <= N; i++) {
			s = (long)dn->Spd_I[i] - (dn->Spd_N[i]>>12);
			dn->Spd_N[i] += x * s;

			if (dn->Spd_N[i] < B_MIN) 
				dn->Spd_N[i] = B_MIN;
		}

		//----- W_BAND_NOISE SUPPRESSION ------04.99
		s_rate_m = dn->S_rate;

		if (voice_threshold > 0)
			s_rate_d = 10; 
		else
			s_rate_d = 20;

		for (i = 0; i <= N; i++) {
			g = dn->Spd_I[i] * dn->Gain_sens_m / dn->Gain_sens_d - 
				(((dn->Gain_nois_m / dn->Gain_nois_d + 1) * dn->Spd_N[i])>>12);

			if (g <= 0)
				g = 0; 
			else
				g = (g <<10) / (g + (dn->Spd_N[i]>>12));

			g *= g;
			g = (g >>10);

			if (g < fpArr_Min) 
				g = fpArr_Min;

			dn->fpArr[i] = g;
		}

		//------ PEAK SUPPRESSION FOR SMOO SPECTRUM ------
		if ( dn->Madsubfi.pict ) {
			//peak suppress condition
			norm = FILTER_NORM + FILTER_NORM / 5;

			for (i = 0; i <= N; i++) {
				//--------- fon fitt ------------
				for (j = 5, sum = 0; j <= 6; j++) {
					mj = abs (i - j);
					kj = i + j;

					if (kj > N)
						kj = N2 - kj;

					sum += (long)dn->Spd_I[kj] + dn->Spd_I[mj]; // time & spec smoo fon
				}
				if (sum < B_MIN)
					sum = B_MIN;

				sum = (sum<<12)* 4 / 10;

				//-------- peak fitt -------------
				p_fact = norm + (sum - dn->Snr[i]) / (sum>>12);
				if (p_fact > FILTER_NORM)
					p_fact = FILTER_NORM;

				if (p_fact < 0)
					p_fact = 0;

				dn->fpArr[i] = (dn->fpArr[i] * p_fact)>>12; //gain control
			}
		} // peak suppress
		s_rate_d = 10;
		s_rate_m = dn->S_rate;
	} else {

		//############ NEW METHODS 2 & 3 ##########################
		tr_1 = fpArr_Norm + fpArr_Norm * (dn->Madsubfi.sensp -18)/80; // 1.07

		//====== Main Circle Spd_N, Snr update ==============
		for (i = 0, j = N2; i <= N; i++, j--) {
			x = dn->Snr[i];//1.07 //save old instant spec
			//--- Spd_I = sqrt (Re ** 2 + Im ** 2) ---
			if (0 == i || i == N) 
				dn->Spd_I[i] = (short)(((long)abs (dn->SigBuf[i]) * SigMax)>>shift);
			else {
				tmpl  = (long)dn->SigBuf[i] * dn->SigBuf[i];
				tmpl += (long)dn->SigBuf[j] * dn->SigBuf[j];
				dn->Spd_I[i]= (short)(((long)i_sqrt(tmpl)*SigMax)>>shift);
			}

			if (dn->Spd_I[i] < B_MIN) 
				dn->Spd_I[i] = B_MIN;

			sum  = (long)(dn->Spd_I[i])<<4;//All Spectrums in FilterNorm Space

			dn->Spd_I[i] = sum;
			if (4 == dn->method)
				dn->Spd_X[i] += (Adap_rate * (sum - dn->Spd_X[i])) >> FILTER_NORM_RATE;

			if (dn->count < 4 && dn->count <= count_max)
				dn->Spd_N[i] = sum;
			//---- Snr[i] update ---------
			if ( sum > 4 * dn->Spd_N[i] )
				dn->Snr[i] = sum;
			else
				dn->Snr[i] += (sum - dn->Snr[i])/2; // * s_rate_m / s_rate_d;

			//---- fast down rate for Spd_N -----
			if (( 2 * x < dn->Spd_N[i] ) && (2 * sum < dn->Spd_N[i] )) 
				dn->Spd_N[i] += ((x + sum)/2 - dn->Spd_N[i]) / 2;//
			else {
				//---- Spd_N update ----------
				if (sum > dn->Spd_N[i]) {
					x = sum/dn->Spd_N[i];
					if (x > 10 )
						x = 10;

					x = Adap_rate / x;
				}	else
					x = Adap_rate;//instant > Spn_N

				//---- FilterNorm were used in Adap_rate ----
				dn->Spd_N[i] += x * (sum - dn->Spd_N[i])>>FILTER_NORM_RATE;
			}
			//------ PEAK SUPPRESSION FOR SMOO SPECTRUM ------
			s = fpArr_Min;//save value

			p_fact = fpArr_Norm;

			if (2 == dn->method) {
				// ====================== METHOD 2 ============================

				// GAIN = 0.25(X/N - Tr) = 0.25(X - Tr x N)/N + Gain_min 
				g = fpArr_Norm * (dn->Snr[i] + dn->Spd_X[i])/2; 
				g = (g / dn->Spd_N[i] - tr_1 )/4 + fpArr_Min;

				if (g < fpArr_Min)
					g = fpArr_Min; 
				else if (g > fpArr_Norm)
					g = fpArr_Norm;

				//----- filtered save for next cadr -------	
				dn->Spd_X[i] = dn->Snr[i] - dn->Spd_N[i];

				dn->fpArr[i] = ( g + dn->fpArr[i] )/2;

			} else if (3 == dn->method) {

				// ====================== METHOD 3 ============================
				//--- GAIN = Gain_min * X/N in fpArr_Norm Space ---
				// fpArr_Min= fpArr_Norm / Gain_min [dn->Madsubfi.supr_max];
				//----------------------------------------------------------
				if (dn->Snr[i] < dn->Spd_N[i] )
					g = p_fact - (p_fact - fpArr_Min) * dn->Snr[i] / dn->Spd_N[i];
				else {
					g = (fpArr_Norm * dn->Snr[i] / dn->Spd_N[i] -tr_1 )/4 + fpArr_Min;
					if (g < fpArr_Min)
						g = fpArr_Min; 
					else if (g > fpArr_Norm)
						g = fpArr_Norm;
				}

				dn->fpArr[i] = ( g + dn->fpArr[i] )/2;
			}

			fpArr_Min = s;//restore Gain_min

		}
	}

	if (4 == dn->method) {
		// ====================== METHOD 4 ============================
#define Gain_MAX   (1 << 3)
		for ( i=dn->i_start, sum=0, s=0; i<=dn->i_fin; i++)
		{

			sum += dn->Spd_I[i];
			s   += dn->Spd_I[i]/ (dn->Spd_X[i]);
		}

		//---  sum >> s => Norma = sum/s > 1 (if s=0 then s =1 )---
		sum = sum / (s+1);//This is Norma
		s = sum / Gain_MAX;//s use for farther simplification of calculations

		//--- Spd_I = invers coeffs in fpArr_Norm space ----
		for (i=0; i<=N; i++)
		{
			//--- SSM (but we may use another SS method !) -----
			if (dn->Snr[i] < dn->Spd_N[i] )
			{
				g = p_fact - (p_fact - fpArr_Min) * dn->Snr[i] / dn->Spd_N[i];
			}
			else
			{
				g = (fpArr_Norm * dn->Snr[i] / dn->Spd_N[i] -tr_1 )/4 + fpArr_Min;

				if (g < fpArr_Min)   g = fpArr_Min; 
				else { if (g > fpArr_Norm) g = fpArr_Norm; }
			}
			//------- END SSM ------------

			//=======  if INVERSION ON ==========

			if ( s > dn->Spd_X[i] )
				tmp = Gain_MAX;
			else
				tmp = sum / (dn->Spd_X[i]);

			g = g * tmp;

			//====== END INVERSION =======

			g = (g + dn->fpArr[i] ) / 2;
			dn->fpArr[i] = g;

		}

		//################ end method 4 ###################
	}

	clear_noice_step_7(dn);

	//--- 8-STEP OF GAIN FITTING: impuls reaction modify with Hann window ---
	x = dn->fpArr[1];
	for (i = 0, j = 1; i < N; i++, j++) {
		s = ((x + (dn->fpArr[i]<<1) + dn->fpArr[j]))>>2;
		x = dn->fpArr[i];
		dn->fpArr[i] = s;
	}

	dn->fpArr[N] = (x + dn->fpArr[N])>>1 ; // used F[N+1] = F[N-1]

	//----------------------------------------------------

	if (0 == dn->method) {
		for (i = 0; i <= N; i++) {
			dn->Spd_X[i] += (dn->fpArr[i] - dn->Spd_X[i]) * s_rate_m / s_rate_d;
			dn->fpArr[i]  =  dn->Spd_X[i];
		}
	}
	//--- 2-STEP OF DATA PROCESS: filter production ------
	for(i = 1, j = N2-1; i < N; i++, j-- ) {
		dn->SigBuf[i] = ((((long)dn->SigBuf[i] * dn->fpArr[i]))>>10); // Re(sig) *= Re(filter)
		dn->SigBuf[j] = ((((long)dn->SigBuf[j] * dn->fpArr[i]))>>10); // Im(sig) *= Re(filter)
	}

	dn->SigBuf[0] = ((((long)dn->SigBuf[0] * dn->fpArr[0]))>>10);
	dn->SigBuf[N] = ((((long)dn->SigBuf[N] * dn->fpArr[N]))>>10);

	//--- 3-STEP OF DATA PROCESS: FFT inverse ------
	SigMaxI  = max_arr (dn->SigBuf, N2);

	shift2 = max_bit_for_invers (dn->fft_order + 1);

	if (SigMaxI)
		for (i = 0; i < N2; i++)
			dn->SigBuf[i] = ((long)(dn->SigBuf[i])<<shift2) / (SigMaxI);

	fft_rev(dn->SigBuf, dn->fft_ldata, dn->fft_sin_rev, dn->fft_order + 1);


	if (SigMaxI)
		for (i = 0; i < N2; i++)
			dn->SigBuf[i] = ((long)dn->SigBuf[i] * SigMaxI)>>shift2;

	norm_sigmax(dn->SigBuf,SigMax,shift,N2);

	if (dn->frame.first) {
		memcpy(dn->OutBuf, dn->SigBuf,         N);
		memcpy(dn->AddBuf, dn->SigBuf + N, Ndiv2);

		dn->frame.first = 0;
	} else {
		// output_data buffer forming
		add_buff (dn->OutBuf + Ndiv2, dn->SigBuf + N + Ndiv2, Ndiv2);

		if (output_data)
			memcpy(output_data, dn->OutBuf, N);

		if (output_data_dim)
			*output_data_dim = N;

		// --------------------------
		memcpy(dn->OutBuf, dn->SigBuf, N);
		add_buff (dn->OutBuf, dn->AddBuf, Ndiv2);
		memcpy(dn->AddBuf, dn->SigBuf + N, Ndiv2);

	}

	if (used)
		*used = N - dn->accumulate_count;

	dn->accumulate_count = 0;
}

