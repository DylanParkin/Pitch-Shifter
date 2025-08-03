#include <iostream>

using namespace std;

#include "portaudio.h"
#include "smbPitchShift.hpp"

const int SAMPLING_FREQ = 44100;
const int BUFFER_SIZE = 256;
const int INPUT_CHANNEL_NO = 1;
const int OUTPUT_CHANNEL_NO = 1;
const PaSampleFormat SAMPLE_FORMAT = paFloat32;
const int DURATION = 60;       // how long to run the loop for (s)
const float SHIFT_AMOUNT = 2;  // 0.5 to 2

static void checkErr(PaError err) {
  if (err != paNoError) {
    printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }
}

int main() {
  // PortAudio setup
  PaStream* stream;
  PaError err;
  err = Pa_Initialize();
  checkErr(err);
  int default_input = Pa_GetDefaultInputDevice();
  int default_output = Pa_GetDefaultOutputDevice();

  if (default_input == paNoDevice || default_output == paNoDevice) {
    cerr << "No default input or output device found." << endl;
    Pa_Terminate();
    return -1;
  }

  err = Pa_OpenDefaultStream(&stream, INPUT_CHANNEL_NO, OUTPUT_CHANNEL_NO,
                             SAMPLE_FORMAT, SAMPLING_FREQ,
                             BUFFER_SIZE,  // Frames per buffer
                             nullptr,      // No callback (using blocking API)
                             nullptr);     // No user data
  checkErr(err);
  err = Pa_StartStream(stream);

  // create input and output audio blocks
  float input_buffer[BUFFER_SIZE];
  float output_buffer[BUFFER_SIZE];

  cout << "Program Start" << endl;

  // loop for N blocks computed using the duration specified and samping freq

  for (int i = 0; i < (DURATION * SAMPLING_FREQ) / BUFFER_SIZE; ++i) {
    // read the stream and store in input buffer.
    err = Pa_ReadStream(stream, input_buffer, BUFFER_SIZE);
    checkErr(err);

    // shift pitch of input buffer and store in output buffer
    smbPitchShift(SHIFT_AMOUNT, BUFFER_SIZE, 1024, 32, SAMPLING_FREQ,
                  input_buffer, output_buffer);

    // write the output buffer to the stream
    err = Pa_WriteStream(stream, output_buffer, BUFFER_SIZE);
    checkErr(err);
  }
  cout << "loop stopped." << endl;

  // Stop input and output streams
  err = Pa_StopStream(stream);
  checkErr(err);

  // Close input and output streams
  err = Pa_CloseStream(stream);
  checkErr(err);

  /// Terminate PortAudio
  Pa_Terminate();

  return 0;
}
