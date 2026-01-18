//
//  main.c
//  StackExchangePolyPhase
//
//  Created by Marvin Harootoonyan on 1/17/26.
//

#include <stdio.h>
#include <math.h>
#include "coef.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"


void convertSampleRate(float* x, float* y, float srcRatio, uint32_t nInputSamples)
    {
    float fixedScaler = 1.0/(float)(0x4000000);
    uint64_t increment = (uint64_t)floor((double)(0x0000000100000000UL)/srcRatio);

    uint32_t nOutputSamples = (uint32_t)floor((double)(nInputSamples-(SRC_FIR_LENGTH-1))*srcRatio);
    uint64_t precisionIndex = 0x0000000100000000UL * (SRC_FIR_LENGTH-1);
    uint32_t prev = 0;
    
    for(uint32_t n=0; n<nOutputSamples; n++)
        {
        uint32_t intIndex = (uint32_t)(precisionIndex>>32);
        int phaseIndex = (int)(precisionIndex>>(32-6)) & (SRC_DECIMATION-1);
        float linearInterpCoef = (float)(precisionIndex & 0x0000000003FFFFFFUL) * fixedScaler;
        
        int iFIR = phaseIndex * SRC_FIR_LENGTH;

        float y0 = 0.0;                 // near linear interpolation value
        uint32_t i = intIndex;
        for (int m=SRC_FIR_LENGTH; m>0; m--)
            {
            y0 += SRC_FIR_coefs[iFIR++] * x[(i--+nInputSamples)%nInputSamples];
            }
        
        float y1 = 0.0;                 // far linear interpolation value
        i = intIndex;                   // reset i but note iFIR just keeps incrementing
        for (int m=SRC_FIR_LENGTH; m>0; m--)
            {
            y1 += SRC_FIR_coefs[iFIR++] * x[(i--+nInputSamples)%nInputSamples];
            }
            
            y[n] = y0 + linearInterpCoef*(y1-y0);
            
    
        precisionIndex += increment;
    }
}

#define OUT_BUFFER_SIZE 524288
float outputBuffer[OUT_BUFFER_SIZE]={0};

int main(int argc, const char * argv[]) {

    
    // WAVE READ
    unsigned int channels;
    unsigned int sampleRate;
   drwav_uint64 totalPCMFrameCount;
    
    
   float* pSampleData = drwav_open_file_and_read_pcm_frames_f32("/Users/marvinharootoonyan/Desktop/StackExchangePolyPhase/StackExchangePolyPhase/saw-256.wav", &channels, &sampleRate, &totalPCMFrameCount, NULL);
    
   if (pSampleData == NULL) {
       // Error opening and reading WAV file.
       printf("error check if you have file");
       return 0;
   }


 
    
    
    
    float srcRatio = 16.0;
    uint32_t nInputSamples = (uint32_t)totalPCMFrameCount;
    uint32_t nOutputSamples = (uint32_t)floor((double)(nInputSamples-(SRC_FIR_LENGTH-1))*(srcRatio));
    int transpose = 0;
    float * walker = outputBuffer;
   
    
    int currentLength = 0;
    
    for(int n =OUT_BUFFER_SIZE; n > 0 && walker != NULL; n-=nOutputSamples){
        nOutputSamples = (uint32_t)floor((double)(nInputSamples-(SRC_FIR_LENGTH-1))*(srcRatio));
        if(currentLength + nOutputSamples < OUT_BUFFER_SIZE){
            convertSampleRate(pSampleData, walker, srcRatio, (uint32_t)(nInputSamples));
            walker = (walker+nOutputSamples);
            
            srcRatio *= 0.991; //transpose to slide through pitch down -> up 
            
            srcRatio = fmaxf(srcRatio, 0.06125); // we might loop for ever for safety;
        }else{
            break;
        }
        currentLength += nOutputSamples;
    }
    
    
    // WAVE WRITE //
    drwav_data_format format;
    format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
    format.format        = DR_WAVE_FORMAT_IEEE_FLOAT;         // <-- Any of the DR_WAVE_FORMAT_* codes.
    format.channels = 1;
    format.sampleRate = 44100;
    format.bitsPerSample = 32;
    drwav wav;
    float t = 0;
    drwav_init_file_write(&wav, "/Users/marvinharootoonyan/Desktop/StackExchangePolyPhase/StackExchangePolyPhase/saw_interpolated.wav", &format, NULL);

    //generate_sine_wave(outputBuffer, 1<<19, 1, 44100.0, 110.0, &t);
    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, OUT_BUFFER_SIZE, outputBuffer);
    
    printf("%d\n",framesWritten);

    drwav_free(pSampleData, NULL);
    drwav_uninit(&wav);
    
       
    
    
    
    return 0;
}
