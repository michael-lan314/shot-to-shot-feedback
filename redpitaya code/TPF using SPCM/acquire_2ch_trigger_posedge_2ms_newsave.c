/* Red Pitaya C API example Acquiring a signal from a buffer
 * This application acquires a signal on a specific channel */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "rp.h"

#define PI 3.141592654
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

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

        rp_AcqReset();
        rp_AcqSetDecimation(RP_DEC_32);	
        //rp_AcqSetDecimationFactor(128);	
	//rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_NE);
        rp_AcqSetTriggerLevel(RP_CH_1, 0.5);




	//rp_AcqSetGain(RP_CH_1, RP_LOW);
	//rp_AcqSetGain(RP_CH_2, RP_HIGH);
	
	double history_cav = 0.0;
	double history_perp = 0.0;
	double output_cav;
	double output_perp;
	double output_A;
	double output_B;
	FILE * f_lastoutput;
    	if ((f_lastoutput = fopen("dontdelete.txt", "r"))) {	

		fscanf(f_lastoutput, "%lf , %lf", &output_cav, &output_perp);
		printf("reading inital value from file! \n output_cav_init = %f. output_perp_init = %f. \n", output_cav, output_perp);	

    		fclose(f_lastoutput);
    	}
	else {
		output_cav = 0.0;
		output_perp = 0.0;

	}


	// read command line arguments to determine which folder to write into
	// get folder path from command line
	char * folder_path;
	if (argc == 2) {
		folder_path = argv[1];
	}
	else {
		printf("ERROR: incorrect number of arguments. \n");
		return 0;
	}

	// add the string RedPitaya to the folder path
	char * folder_path_rp = malloc(100);
	sprintf(folder_path_rp, "%s/RedPitaya", folder_path);
	//folder_path = folder_path_rp;
	
	// create folder if it doesn't exist
	char command[100];
	sprintf(command, "mkdir -p %s", folder_path_rp);
	system(command);
	// change directory to folder
	chdir(folder_path_rp);
	// create files to write to
	FILE * fpa;
	FILE * fpb;
	fpa = fopen ("ch1.txt", "w+"); // open the file output is stored
	fpb = fopen ("ch2.txt", "w+");


	FILE * f_phicav;
	FILE * f_histcav;
	FILE * f_phiperp;
	FILE * f_histperp;
	FILE * f_outcav;
	FILE * f_outperp;
	FILE * f_lencav;
	FILE * f_lenperp;
	FILE * f_cnstcav;
	FILE * f_cnstperp;
	//f_phicav = fopen ("phicav0.txt", "w"); 
	//f_histcav = fopen ("histcav0.txt", "w");
	//f_phiperp = fopen ("phiperp0.txt", "w"); 
	//f_histperp = fopen ("histperp0.txt", "w");

	//############################user input##############################
	//float cav_to_x = 0.71;
	//float perp_to_x = 0.93;
	//float cav_to_z = 1.71;
	//float perp_to_z = -1.57;

	float cav_to_x = 0.81;
	float perp_to_x = 0.93;
	float cav_to_z = 1.52;
	float perp_to_z = -1.57;


	float unittime = 4192.0/16384;

	float gain_cav = -0.12;
	float gain_perp = -0.16;
	float decay = 0.5;
	int memlen = 3; //<= 20
	float order = 1;
	
	// float phi_cav_offset = 0;
	// float phi_perp_offset = 0;
	float phi_cav_offset = -0.2;
	float phi_perp_offset = -0.2;
	//####################################################################


	int ts_len_cav[20] = { 0 };
	float ts_all_cav[20][2000];

	int ts_len_perp[20] = { 0 };
	float ts_all_perp[20][2000];


	for(int epoch = 0; epoch < 100000; epoch++){
		f_phicav = fopen ("phicav.txt", "a"); 
		f_histcav = fopen ("histcav.txt", "a");
		f_outcav = fopen ("outcav.txt", "a");
		f_lencav = fopen ("lencav.txt", "a");
		f_cnstcav = fopen ("cnstcav.txt", "a");
		f_phiperp = fopen ("phiperp.txt", "a"); 
		f_histperp = fopen ("histperp.txt", "a");
		f_outperp = fopen ("outperp.txt", "a");
		f_lenperp = fopen ("lenperp.txt", "a");
		f_cnstperp = fopen ("cnstperp.txt", "a");


        	float *buff_ch1 = (float *)malloc(buff_size * sizeof(float));
        	float *buff_ch2 = (float *)malloc(buff_size * sizeof(float));

	        rp_AcqStart();

	        /* After acquisition is started some time delay is needed in order to acquire fresh samples in to buffer*/
	        /* Here we have used time delay of one second but you can calculate exact value taking in to account buffer*/
	        /*length and smaling rate*/

	        sleep(0.6);
	        rp_AcqSetTriggerSrc(RP_TRIG_SRC_CHA_PE);
	        rp_acq_trig_state_t state = RP_TRIG_STATE_TRIGGERED;

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

		/*
       		int counter = 0;
		float numerator = 0.0;
		float denominator = 0.0;
		//float A_sum = 0.0;
		float B_sum = 0.0;
        	for(int i = trig_time; i < buff_size; i++){
                		//if (i % 400 == 0) printf("%f  %f\n", buff_ch1[i], buff_ch2[i]);
			if (buff_ch1[i] >0.1) counter++;
				//numerator += i * (buff_ch2[i]-B_baseline);
				//denominator += (buff_ch2[i]-B_baseline);

			numerator += i * (buff_ch2[i]-B_baseline) * (buff_ch2[i]-B_baseline);
			denominator += (buff_ch2[i]-B_baseline) * (buff_ch2[i]-B_baseline);
			//A_sum +=  buff_ch1[i]-A_baseline;
			B_sum +=  buff_ch2[i]-B_baseline;
			//fprintf(fpa, "%f ", buff_ch1[i]);
			//fprintf(fpb, "%f ", buff_ch2[i]);
        	}

		double center = unittime * (numerator/denominator - center_finetuning);
		double delta_PC = unitfreq * (numerator/denominator - center_finetuning);
		float intensity = denominator;
		//fclose(fpa);
		//fclose(fpb);
		*/



				
		//############################GETTING TIMESTAMPS##############################
		//float timestamp_cav[1000]; //this is an array of number from 0 to 1
		bool saturate = false;
		int len_timestamp_cav = 0;
		for(int i = 8200; i < buff_size-1; i++){
			if (buff_ch2[i+1] - buff_ch2[i] > 0.05) {
				//timestamp_cav[len_timestamp_cav] = (i+1-8200) * unittime / 2000.0;
				ts_all_cav[(epoch % memlen)][len_timestamp_cav] = (i+1-8200) * unittime / 2000.0;
				len_timestamp_cav += 1;
			}
			if (buff_ch2[i+1]>0.8 && buff_ch2[i-10]>0.8 && buff_ch2[i-20]>0.8 && buff_ch2[i-30]>0.8 && buff_ch2[i-40]>0.8) {
				saturate = true;
			}
		}
		ts_len_cav[(epoch % memlen)] = len_timestamp_cav;

		//float timestamp_perp[1000]; //this is an array of number from 0 to 1
		int len_timestamp_perp = 0;
		for(int i = 400; i < 8200; i++){
			if (buff_ch2[i+1] - buff_ch2[i] > 0.05) {
				//timestamp_perp[len_timestamp_perp] = (i+1-400) * unittime / 2000.0;
				ts_all_perp[(epoch % memlen)][len_timestamp_perp] = (i+1-400) * unittime / 2000.0;
				len_timestamp_perp += 1;
			}
			if (buff_ch2[i+1]>0.8 && buff_ch2[i-10]>0.8 && buff_ch2[i-20]>0.8 && buff_ch2[i-30]>0.8 && buff_ch2[i-40]>0.8) {
				saturate = true;
			}

		}
		ts_len_perp[(epoch % memlen)] = len_timestamp_perp;

		
				
		//############################fitting phases##############################
		int phi_decimation = 40;
		//int phiarray[phi_decimation];
		float phi;  
		float sum;

		double sum_cav_max = -1000.0;
		double sum_cav_min = 10000000.0;

		double phi_cav_argmax;
		for (int j = 0; j < phi_decimation; j++){
			phi = ((float)j)/phi_decimation - 0.5;
			//printf(" %f ", phi);
			sum = 0;
			for (int j = 0; j < memlen; j++){
				for (int i = 0; i < ts_len_cav[j]; i++){
					sum += 1+ cosf(2*PI * (ts_all_cav[j][i]+phi+phi_cav_offset));
				}
			}
			if (sum>sum_cav_max){
				sum_cav_max = sum;
				phi_cav_argmax = phi;
			}
			if (sum<sum_cav_min) sum_cav_min = sum;
		}

		//printf(" %f ", phi_cav_argmax);
		float sum_perp_max = -1000.0;
		float sum_perp_min = 10000000.0;
		float phi_perp_argmax;
		for (int j = 0; j < phi_decimation; j++){
			phi =((float)j)/phi_decimation - 0.5;
			sum = 0;
			for (int j = 0; j < memlen; j++){
				for (int i = 0; i < ts_len_perp[j]; i++){
					sum += 1+ cosf(2*PI * (ts_all_perp[j][i]+phi+phi_perp_offset));
				}
			}
			if (sum>sum_perp_max){
				sum_perp_max = sum;
				phi_perp_argmax = phi;
			}
			if (sum<sum_perp_min) sum_perp_min = sum;
		}
			
		float contrast_cav = 2*(sum_cav_max - sum_cav_min) / (sum_cav_max + sum_cav_min);
		float contrast_perp = 2*(sum_perp_max - sum_perp_min) / (sum_perp_max + sum_perp_min);
		
				
		//############################defining a valid run##############################

		bool valid_run = true;
		
		if (len_timestamp_cav < 60 && len_timestamp_perp < 60) valid_run = false;
		if (saturate) valid_run = false;


		//####################################################################


		


		if (valid_run){
			//if (phi_cav_argmax > history_cav+0.5) phi_cav_argmax -= 1;
			//if (phi_cav_argmax < history_cav-0.5) phi_cav_argmax += 1;
			history_cav = decay * history_cav + (1-decay) * phi_cav_argmax;
			output_cav += pow(fabs(10.0*history_cav),order-1) *history_cav* gain_cav;
			//output_cav += history_cav* gain_cav;

			//output_cav = 0.125*(epoch%5) ;


			//if (phi_perp_argmax > history_perp+1) phi_perp_argmax -= 1;
			//if (phi_perp_argmax < history_perp-1) phi_perp_argmax += 1;
			history_perp = decay * history_perp + (1-decay) * phi_perp_argmax;
			output_perp += pow(fabs(10.0*history_perp),order-1) *history_perp* gain_perp;
			//output_perp += history_perp* gain_perp;
			//output_perp = 0;

			output_A = cav_to_x*output_cav + perp_to_x*output_perp; //x_pzt
			output_B = cav_to_z*output_cav + perp_to_z*output_perp; //z_pzt
		}




		if (output_A>1){
			printf("x_pzt output Hitting upper rail! ");
			output_perp -= 1;
			output_cav -= 1;
			output_A -= cav_to_x + perp_to_x;
			output_B -= (cav_to_z + perp_to_z);
		}
		if (output_A<-1){
			printf("x_pzt output Hitting lower rail! ");
			output_perp += 1;
			output_cav += 1;
			output_A += cav_to_x + perp_to_x;
			output_B += (cav_to_z + perp_to_z);
		}

		if (output_B>1){
			printf("z_pzt output Hitting upper rail! ");
			if (output_A>-0.1) {output_cav -= 1; output_A -= cav_to_x; output_B -= cav_to_z; }
			else  {output_perp += 1; output_A += perp_to_x; output_B += perp_to_z; }

		}
		if (output_B<-1){
			printf("z_pzt output Hitting lower rail! ");
			output_B += 1.54;
			if (output_A>0.1) {output_perp -= 1; output_A -= perp_to_x; output_B -= perp_to_z; }
			else  {output_cav += 1; output_A += cav_to_x; output_B += cav_to_z; }
		}




/*

		if (output_A>1){
			printf("x_pzt output Hitting upper rail! ");
			output_perp -= 1.54;
			output_cav -= 1.54;
			output_A -= 1.88;
		}
		if (output_A<-1){
			printf("x_pzt output Hitting lower rail! ");
			output_perp += 1.54;
			output_cav += 1.54;
			output_A += 1.88;
		}

		if (output_B>1){
			printf("z_pzt output Hitting upper rail! ");
			output_B -= 1.54;
			if (output_A>-0.1) {output_A -= 0.83; output_cav -= 1.54;}
			else  {output_A += 1.04; output_perp += 1.54;}

		}
		if (output_B<-1){
			printf("z_pzt output Hitting lower rail! ");
			output_B += 1.54;
			if (output_A>0.1) {output_A -= 1.04; output_perp -= 1.54;}
			else  {output_A += 0.83; output_cav += 1.54;}
		}
*/


        	rp_GenReset();        	
		if (output_A > 0) rp_GenWaveform(RP_CH_1, RP_WAVEFORM_DC);
		else rp_GenWaveform(RP_CH_1, RP_WAVEFORM_DC_NEG);
        	rp_GenAmp(RP_CH_1, fabs(output_A));

		if (output_B > 0) rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC);
		else rp_GenWaveform(RP_CH_2, RP_WAVEFORM_DC_NEG);
        	rp_GenAmp(RP_CH_2, fabs(output_B));


        	rp_GenOutEnable(RP_CH_1);
        	rp_GenOutEnable(RP_CH_2);

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

		time_t seconds;
     		seconds = time(NULL);
		printf ( "\n Current absolute time: %ld \n", seconds );

		if (valid_run){
			printf("valid run. run name %s, shot number %d \n", folder_path, epoch);
			printf("len_cav = %d, len_perp = %d \n", len_timestamp_cav, len_timestamp_perp);
			printf("phi_cav = %f, phi_perp = %f. \n", phi_cav_argmax, phi_perp_argmax);

			printf("contrast_cav = %f, contrast_perp = %f \n", contrast_cav, contrast_perp);

			printf("history_cav = %f, history_perp = %f. \n", history_cav, history_perp);

			printf("output_cav = %f. output_perp = %f. \n", output_cav, output_perp);	

			printf("output_A = %f. output_B = %f. \n", output_A, output_B);	
	
			//for(int i = 0; i < buff_size; i++){
			//	fprintf(fpa, "%f ", buff_ch1[i]);
			//	fprintf(fpb, "%f ", buff_ch2[i]);
			//}
			//fprintf(fpa, "\n");
			//fprintf(fpb, "\n");
		}
		else {
			printf("INVALID run!!!  number %d \n", epoch);
			if (len_timestamp_cav < 100) printf("cav scan has too few photons = %d.\n", len_timestamp_cav);
			if (len_timestamp_perp < 100) printf("perp scan has too few photons = %d.\n", len_timestamp_perp);
			if (saturate) printf("SPCM saturated!!!\n");
			//printf("\n");

			//printf("trigger filled %d out of %d! \n \n", counter, buff_size);
        		//for(int i = 0; i < buff_size; i++){
				//fprintf(fpa, "%f ", buff_ch1[i]);
				//fprintf(fpb, "%f ", buff_ch2[i]);
        		//}
			//fprintf(fpa, "\n");
			//fprintf(fpb, "\n");

		}		
		//for(int i = 0; i <  buff_size; i++){
			//fprintf(f_phicav, "%f ", buff_ch1[i]);
			//fprintf(fpb, "%f ", buff_ch2[i]);
			//}
		//fprintf(f_phicav, "\n");
		//fprintf(fpb, "\n");
		printf("*************************");
		fprintf(f_phicav, "%ld , %f \n", seconds, phi_cav_argmax);
		fprintf(f_phiperp, "%ld , %f  \n", seconds, phi_perp_argmax);
		fprintf(f_histcav, "%ld , %f  \n", seconds, history_cav);
		fprintf(f_histperp, "%ld , %f  \n", seconds, history_perp);
		fprintf(f_outcav, "%ld , %f  \n", seconds, output_cav);
		fprintf(f_outperp, "%ld , %f  \n", seconds, output_perp);
		fprintf(f_lencav, "%ld , %d  \n", seconds, len_timestamp_cav);
		fprintf(f_lenperp, "%ld , %d  \n", seconds, len_timestamp_perp);
		fprintf(f_cnstcav, "%ld , %f  \n", seconds, contrast_cav);
		fprintf(f_cnstperp, "%ld , %f  \n", seconds, contrast_perp);
		printf("*************************");


		/*
		for(int i = 0; i < buff_size; i++){
			fprintf(fpa, "%f ", buff_ch1[i]);
			fprintf(fpb, "%f ", buff_ch2[i]);
			}
		fprintf(fpa, "\n");
		fprintf(fpb, "\n");
		*/


		
	        /* Releasing resources */
	        free(buff_ch1);
	        free(buff_ch2);
		fclose(f_phicav);
		fclose(f_phiperp);
		fclose(f_histcav);
		fclose(f_histperp);
		fclose(f_outcav);
		fclose(f_outperp);
		fclose(f_lencav);
		fclose(f_lenperp);
		fclose(f_cnstcav);
		fclose(f_cnstperp);

		if (valid_run){
 
    			chdir("..");
     			chdir(".."); 
			f_lastoutput = fopen ("dontdelete.txt", "w+");
			fprintf(f_lastoutput, "%f , %f  \n", output_cav, output_perp);
			fclose(f_lastoutput);
			chdir(folder_path_rp);
		}


	}	        
	rp_Release();
	//fclose(f_phicav);
	//fclose(f_phiperp);
	//fclose(f_histcav);
	//fclose(f_histperp);
	fclose(fpa);
	fclose(fpb);


        return 0;

}
