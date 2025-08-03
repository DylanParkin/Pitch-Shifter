#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

#include "portaudio.h"
#include "smbPitchShift.hpp"

const int SAMPLING_FREQ = 44100;
const int BUFFER_SIZE = 256;
const int INPUT_CHANNEL_NO = 1;
const int OUTPUT_CHANNEL_NO = 1;
const PaSampleFormat SAMPLE_FORMAT = paFloat32;
const int NUM_THREADS = 3;
const int DURATION = 60;       // how long to run the loop for
const float SHIFT_AMOUNT = 2;  // 0.5 to 2

const int RING_SIZE = 10;
const int num_blocks = (DURATION * SAMPLING_FREQ) / BUFFER_SIZE;

// track the state of each slot
enum class SlotStatus { EMPTY, FILLED, PROCESSED };

// define a ring buffer slot
struct ring_buffer_slot {
  float block[BUFFER_SIZE];
  mutex mtx;
  SlotStatus status = SlotStatus::EMPTY;
  condition_variable cv;
};

// make ring buffer to store blocks of data and their locks
ring_buffer_slot ring_buffer[RING_SIZE];

static void checkErr(PaError err) {
  if (err != paNoError) {
    printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }
}

// funciton ran by reader thread to read blocks from the stream
// waits for notification and slot to be empty
void ReadStream(PaStream* stream, ring_buffer_slot* ring_buffer) {
  for (int read_index = 0; read_index < num_blocks; read_index++) {
    int slot = read_index % RING_SIZE;  // wraps

    unique_lock<mutex> lock(ring_buffer[slot].mtx);  // unique lock for wait()

    // unlock and wait till notified AND slot empty
    ring_buffer[slot].cv.wait(lock, [&ring_buffer, slot]() -> bool {
      return ring_buffer[slot].status == SlotStatus::EMPTY;
    });

    Pa_ReadStream(stream, ring_buffer[slot].block, BUFFER_SIZE);

    // update status and notify the waiting thread
    ring_buffer[slot].status = SlotStatus::FILLED;
    ring_buffer[slot].cv.notify_one();
  }
}

// function ran by procssing thread to apply pitch shift to a block.
// waits for notification and slot to be filled
void ProcessBlock(ring_buffer_slot* ring_buffer) {
  for (int process_index = 0; process_index < num_blocks; process_index++) {
    int slot = process_index % RING_SIZE;

    unique_lock<mutex> lock(ring_buffer[slot].mtx);

    ring_buffer[slot].cv.wait(lock, [&ring_buffer, slot]() -> bool {
      return ring_buffer[slot].status == SlotStatus::FILLED;
    });

    smbPitchShift(SHIFT_AMOUNT, BUFFER_SIZE, 1024, 32, SAMPLING_FREQ,
                  ring_buffer[slot].block, ring_buffer[slot].block);

    ring_buffer[slot].status = SlotStatus::PROCESSED;
    ring_buffer[slot].cv.notify_one();
  }
}

// function ran by writing thread to apply pitch shift to a block.
// waits for notification and slot to be processed
void WriteBlock(PaStream* stream, ring_buffer_slot* ring_buffer) {
  for (int write_index = 0; write_index < num_blocks; write_index++) {
    int slot = write_index % RING_SIZE;

    unique_lock<mutex> lock(ring_buffer[slot].mtx);

    ring_buffer[slot].cv.wait(lock, [&ring_buffer, slot]() -> bool {
      return ring_buffer[slot].status == SlotStatus::PROCESSED;
    });

    Pa_WriteStream(stream, ring_buffer[slot].block, BUFFER_SIZE);

    ring_buffer[slot].status = SlotStatus::EMPTY;
    ring_buffer[slot].cv.notify_one();
  }
}

int main() {
  // PortAudio setup
  PaStream* stream;
  PaError err;
  err = Pa_Initialize();
  checkErr(err);
  int defaultInputDevice = Pa_GetDefaultInputDevice();
  int defaultOutputDevice = Pa_GetDefaultOutputDevice();

  if (defaultInputDevice == paNoDevice || defaultOutputDevice == paNoDevice) {
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

  cout << "Program Start" << endl;

  // spawn threads
  thread threads[NUM_THREADS];

  threads[0] = thread(ReadStream, stream, ring_buffer);
  threads[1] = thread(ProcessBlock, ring_buffer);
  threads[2] = thread(WriteBlock, stream, ring_buffer);

  // join threads
  threads[0].join();
  threads[1].join();
  threads[2].join();

  cout << "Program End" << endl;
  err = Pa_StopStream(stream);
  checkErr(err);
  err = Pa_CloseStream(stream);
  checkErr(err);
  Pa_Terminate();
  return 0;
}
