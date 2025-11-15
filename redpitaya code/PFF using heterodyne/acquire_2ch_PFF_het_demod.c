/* Red Pitaya C API example Acquiring a signal from a buffer
 * This application acquires a signal on a specific channel */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "rp.h"

int main(int argc, char **argv){

        /* Print error, if rp_Init() function failed */
        if(rp_Init() != RP_OK){
                fprintf(stderr, "Rp api init failed!\n");
        }

        /*LOOB BACK FROM OUTPUT 2 - ONLY FOR TESTING*/
        //rp_GenReset();
        //rp_GenFreq(RP_CH_1, 20000.0);
        //rp_GenAmp(RP_CH_1, 1.0);
        //rp_GenWaveform(RP_CH_1, RP_WAVEFORM_SINE);
        //rp_GenOutEnable(RP_CH_1);


        uint32_t buff_size = 16384;

        uint32_t buff_size_trim = 8192;

        rp_AcqReset();
        rp_AcqSetDecimation(RP_DEC_32);	
	//rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_NE);
        rp_AcqSetTriggerLevel(RP_CH_1, 0.7);




	//rp_AcqSetGain(RP_CH_1, RP_LOW);
	//rp_AcqSetGain(RP_CH_2, RP_HIGH);
	
	double history = 0.0;
	double output = 0.0;



	FILE * fpa;
	FILE * fpb;
	fpa = fopen ("ch1.txt", "w+"); // open the file output is stored
	fpb = fopen ("ch2.txt", "w+");


	while(1){
        	float *buff_ch1 = (float *)malloc(buff_size * sizeof(float));
        	float *buff_ch2 = (float *)malloc(buff_size * sizeof(float));

	        rp_AcqStart();

	        /* After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer*/
	        /* Here we have used time delay of one second but you can calculate exact value taking in to account buffer*/
	        /*length and smaling rate*/

	        sleep(0.6);
	        rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
	        rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;
	        //rp_acq_trig_state_t state = RP_TRIG_STATE_WAITING;

	        while(1){
	                rp_AcqGetTriggerState(&state);
	                if(state == RP_TRIG_STATE_TRIGGERED){
	                break;
	                }
	        }


	        //rp_AcqGetOldestDataV(RP_CH_1, &buff_size, buff_ch1);


	        uint32_t pos = 0;
	        rp_AcqGetWritePointerAtTrig(&pos);
	        rp_AcqGetDataV2(pos, &buff_size, buff_ch1,buff_ch2);
		clock_t begin = clock();
		//############################user input##############################

        float f_het = 0.5008f;  // <-- user input: heterodyne frequency (MHz)

		//float B_baseline = 0.25;
		//float A_baseline = -0.036;	
		float freq_sweep = 4; //MHz
		float time_sweep = 2000; //us
		// float unittime = 4192.0/16384;
		float unittime = 32.0f/125.0f;
        // RP_DEC_32 -> f_s
        float fs = 125.0f / 32.0f;  // MHz
		//float unitfreq = unittime * freq_sweep/time_sweep;
		int trig_time = 100/unittime;
		// float center_finetuning = 3870;
		float center_finetuning = 3845;

		float gain = 0.07;
		float decay = 0.7;
		//####################################################################



        // ############################ DEMOD het ##############################
        bool saturate = false;

        // ---------------- DEMOD het with alias handling ----------------
        int requested_num_bins = 40;   // number of bins
        int num_bins = requested_num_bins;


        // your current truncated buffer size
        int total_samples = buff_size_trim - trig_time;

        // float unittime = 1.0f / fs; // µs per sample

        // compute aliased frequency in sampled data
        float f_mod = f_het - floorf(f_het / fs) * fs;
        float f_alias = (f_mod > fs / 2.0f) ? (fs - f_mod) : f_mod;
        if (f_alias < 1e-12f) f_alias = 0.0f; // avoid 0

        // samples per cycle of aliased frequency
        float samples_per_cycle = (f_alias > 0.0f) ? (fs / f_alias) : (float)total_samples;



	// 4. Cycles per bin (float!)
	float cycles_per_bin = (float)total_samples / (num_bins * samples_per_cycle);

	// 5. Number of samples per bin (integer)
	int num_per_bin = (int)roundf(cycles_per_bin * samples_per_cycle);
	if (num_per_bin < 1) num_per_bin = 1;

	// 6. Truncate tail
	int usable_samples = (total_samples / num_per_bin) * num_per_bin;
	int total_bins = usable_samples / num_per_bin;
	if (total_bins <= 0) {
    		num_per_bin = total_samples;
    		total_bins = 1;
	}

        // debug
        printf("f_het=%.4f MHz, f_alias=%.4f MHz, samples_per_cycle=%.4f\n", f_het, f_alias, samples_per_cycle);
        // printf("requested_num_bins=%d -> cycles_per_bin=%.4f, num_per_bin=%d samples, total_bins=%d, usable_samples=%d\n", requested_num_bins, cycles_per_bin, num_per_bin, total_bins, usable_samples);

        // phase increment per sample
        float phase_inc = 2.0f * M_PI * f_alias * unittime;
        float phase = trig_time * phase_inc;

        // demodulate into bins
        float amp_bin[50] = {0}; // or dynamically allocate total_bins
        for (int b = 0; b < total_bins; b++) {
            float i_sum = 0.0f, q_sum = 0.0f;
            int start = trig_time + b * num_per_bin;
            int end = start + num_per_bin;
            for (int i = start; i < end; i++) {
                float val = buff_ch2[i];
                float cosv = cosf(phase);
                float sinv = sinf(phase);
                i_sum += val * cosv;
                q_sum += val * sinv;
                phase += phase_inc;
                if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
            }
            amp_bin[b] = (i_sum * i_sum + q_sum * q_sum); // squared amplitude
        }

        // compute weighted center
        float weighted_sum_temp = 0.0f;
        float sum_temp = 0.0f;
        for (int i = 0; i < total_bins; i++) {
            float time_fraction = (i * num_per_bin * unittime) / time_sweep; // normalized 0..1 across sweep
            weighted_sum_temp += amp_bin[i] * time_fraction;
            sum_temp += amp_bin[i];
        }
        float avg_temp = (sum_temp > 0.0f) ? (weighted_sum_temp / sum_temp) : 0.0f;
        double center = avg_temp - center_finetuning * unittime / time_sweep;
        double delta_CP = freq_sweep * center;

        // debug print
        printf("avg_temp=%.6f center=%.6f delta_CP=%.6f\n", avg_temp, center, delta_CP);



        // int num_bins = 50;

        // // Compute how many samples per heterodyne cycle
        // float samples_per_cycle = 1.0f / (f_het * unittime);  // f_het in MHz, unittime in µs

        // // Estimate cycles per bin, then round to nearest integer
        // int total_samples = buff_size_trim - trig_time;
        // float est_cycles_per_bin = (float)total_samples / (num_bins * samples_per_cycle);
        // int cycles_per_bin = (int)(est_cycles_per_bin + 0.5f);
        // if (cycles_per_bin < 1) cycles_per_bin = 1;

        // // Compute integer bin width (samples)
        // int num_per_bin = (int)(cycles_per_bin * samples_per_cycle);

        // // Truncate to full bins only
        // int usable_samples = (total_samples / num_per_bin) * num_per_bin;
        // int total_bins = usable_samples / num_per_bin;

        // // Debug print
        // printf("f_het=%.3f MHz, samples_per_cycle=%.2f, cycles_per_bin=%d, num_per_bin=%d, bins=%d\n",
        //     f_het, samples_per_cycle, cycles_per_bin, num_per_bin, total_bins);

        // // Allocate and compute amplitudes
        // float amp_bin[50] = {0};
        // float phase_inc = 2 * M_PI * f_het * unittime;
        // float phase = trig_time * phase_inc;

        // for (int b = 0; b < total_bins; b++) {
        //     float i_sum = 0.0f, q_sum = 0.0f;
        //     int start = trig_time + b * num_per_bin;
        //     int end = start + num_per_bin;
        //     for (int i = start; i < end; i++) {
        //         float val = buff_ch2[i];
        //         float cosv = cosf(phase);
        //         float sinv = sinf(phase);
        //         i_sum += val * cosv;
        //         q_sum += val * sinv;
        //         phase += phase_inc;
        //         if (phase > 2 * M_PI)
        //             phase -= 2 * M_PI;
        //     }
        //     amp_bin[b] = (i_sum * i_sum + q_sum * q_sum);
        // }

        // // ############################### Getting center from 14 MHz amplitude ######################
        // float weighted_sum_temp = 0.0f;
        // float sum_temp = 0.0f;

        // for (int i = 0; i < num_bins; i++) {
        //     // Convert bin index to normalized time fraction (0 → 1 over full sweep)
        //     float time_fraction = (i * num_per_bin * unittime) / time_sweep;
        //     weighted_sum_temp += amp_bin[i] * time_fraction;
        //     sum_temp += amp_bin[i];
        // }

        // // Compute weighted average (center of amplitude distribution)
        // float avg_temp = weighted_sum_temp / sum_temp;

        // // Center correction and conversion to detuning
        // double center = avg_temp - center_finetuning * unittime / time_sweep;
        // double delta_CP = freq_sweep * center;
        // ############################################################################

/*


 		for(int i=buff_size_trim+trig_time;i<buff_size;i++) 
		{ 
			sum_temp+=buff_ch2[i]; 
		} 
		B_baseline =sum_temp/(buff_size-buff_size_trim-1000); 
		
		bool saturate = false;

        	for(int i = trig_time; i < buff_size_trim; i++){
                		//if (i % 400 == 0) printf("%f  %f\n", buff_ch1[i], buff_ch2[i]);
			if (buff_ch1[i] >0.1) counter++;
				//numerator += i * (buff_ch2[i]-B_baseline);
				//denominator += (buff_ch2[i]-B_baseline);

			numerator += i * (buff_ch2[i]-B_baseline) * (buff_ch2[i]-B_baseline);
			denominator += (buff_ch2[i]-B_baseline) * (buff_ch2[i]-B_baseline);
			//A_sum +=  buff_ch1[i]-A_baseline;
			B_sum +=  buff_ch2[i]-B_baseline;

			if (buff_ch2[i+1]>0.8 && buff_ch2[i-10]>0.8 && buff_ch2[i-20]>0.8 && buff_ch2[i-30]>0.8 && buff_ch2[i-40]>0.8) {
				saturate = true;
			}

			//fprintf(fpa, "%f ", buff_ch1[i]);
			//fprintf(fpb, "%f ", buff_ch2[i]);
        	}

		double center = unittime * (numerator/denominator - center_finetuning);
		double delta_CP = unitfreq * (numerator/denominator - center_finetuning);
		float intensity = denominator;
		//fclose(fpa);
		//fclose(fpb);
		

*/

		bool valid_run = true;
				
		//############################defining a valid run timestamp##############################
		/*
		if (saturate) valid_run = false;
		if (len_timestamp_cav<20) valid_run = false;
		*/
		//####################################################################
		//############################defining a valid run pi lowpass##############################
		/*
		float thresh = 0.2;
		int front = 0;
		for(int i = trig_time; i < trig_time+1000; i++){
			if (buff_ch2[i]>thresh) front += 1;
		}
		int rear = 0;

		for(int i = buff_size_trim-1000; i < buff_size_trim; i++){
			if (buff_ch2[i]>thresh) rear += 1;
		}
		
		if (front < 20 && rear < 20) valid_run = false;
		if (front == 0 || rear == 0) valid_run = false;
		*/
		//####################################################################

		//############################defining a valid run 2db##############################
		/*
		float thresh = 0.2;
		int front = 0;
		for(int i = trig_time; i < trig_time+1000; i++){
			if (buff_ch2[i]>thresh) front += 1;
		}
		int rear = 0;

		for(int i = buff_size_trim-1000; i < buff_size_trim; i++){
			if (buff_ch2[i]>thresh) rear += 1;
		}
		
		if (front < 4 && rear < 4) valid_run = false;
		//if (front == 0 || rear == 0) valid_run = false;
		if (saturate) valid_run = false;
		*/
		//####################################################################



		


		if (valid_run){
			history = decay * history + center;
			output += history * gain;
		}


 
		//output = 0;
		if (output>1){
			printf("Hitting upper rail! ");
			output = 1;
		}
		if (output<-1){
			printf("Hitting lower rail! ");
			output = -1;
		}


        	rp_GenReset();        	
		if (output > 0) rp_GenWaveform(RP_CH_1, RP_WAVEFORM_DC);
		else rp_GenWaveform(RP_CH_1, RP_WAVEFORM_DC_NEG);
        	rp_GenAmp(RP_CH_1, fabs(output));
        	rp_GenOutEnable(RP_CH_1);

		/* #####################################################
		int status = rp_AOpinSetValue(0, 1.0+output);
        	if (status != RP_OK) {
			int i = 0;
            		printf("Could not set AO[%i] voltage.\n", i);
        	}
		####################################################### */

		clock_t end = clock();
		double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		printf("\n time spent = %f ", time_spent);
		//printf("\n");
		//printf("number of nonzero values: %d out of %d \n", counter, buff_size);
		if (valid_run){
			printf("valid run! \n");
			printf("delta_CP = %f MHz. ", delta_CP);
			//printf("count = %d, ", len_timestamp_cav);
			printf("pseudo-count = %.2f, ", sum_temp*50);
			printf("history = %f. \n", history);
			printf("output = %f. \n", output);	
	
			//for(int i = 0; i < buff_size; i++){
			//	fprintf(fpa, "%f ", buff_ch1[i]);
			//	fprintf(fpb, "%f ", buff_ch2[i]);
			//}
			//fprintf(fpa, "\n");
			//fprintf(fpb, "\n");
		}
		else {
			printf("INVALID run!!! \n");
			//if (front==0) printf("front of sequnce has no signal.\n");
			//if (rear==0) printf("rear of sequnce has no signal.\n");
			if (sum_temp<2) printf("low count = %.2f. cavity most likely unlocked.\n", sum_temp);
			if (saturate) printf("SPCM saturated.\n");
			//printf("\n");

			//printf("trigger filled %d out of %d! \n \n", counter, buff_size);
        		//for(int i = 0; i < buff_size; i++){
				//fprintf(fpa, "%f ", buff_ch1[i]);
				//fprintf(fpb, "%f ", buff_ch2[i]);
        		//}
			//fprintf(fpa, "\n");
			//fprintf(fpb, "\n");

		}		
		//for(int i = 0; i < buff_size; i++){
		//	fprintf(fpa, "%f ", buff_ch1[i]);
		//	fprintf(fpb, "%f ", buff_ch2[i]);
		//	}
		//fprintf(fpa, "\n");
		//fprintf(fpb, "\n");

		
	        /* Releasing resources */
	        free(buff_ch1);
	        free(buff_ch2);


	}	        
	rp_Release();
	fclose(fpa);
	fclose(fpb);

        return 0;

}
