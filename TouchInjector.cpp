// Copyright(c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma comment(lib, "winmm.lib")
#include <Windows.h>
#include <iostream>

// Milliseconds per second.
constexpr uint32_t MS_PER_SEC = 1000;
// Microseconds per second.
constexpr uint32_t US_PER_SEC = MS_PER_SEC * MS_PER_SEC;
// Time interval in QPC units between the injected touch inputs.
uint64_t injection_interval_in_qpc_units;

long Interpolate(long start, long end, double ratio) {
  return static_cast<long>(start + (end - start) * ratio);
}

double InjectionIntervalInMsFromFrequency(int frequency) {
  return (static_cast<double>(1.0) / static_cast<double>(frequency)) *
         MS_PER_SEC;
}

// Calculates the number of packets to inject based on given pan
// duration and injection interval.
uint32_t CalculatePacketsNeeded(long duration, double interval) {
  uint32_t packet_count = static_cast<uint32_t>(duration / interval);
  // Rounding up the number of packets.
  // Why rounding up? Let say we want to pan 500 pixels in 1 sec at frame
  // frequency of 60. In this case, each frame/input will be injected at an
  // interval of 16ms. As per the above code, |packet_count| = 62. Now, 62*16ms
  // equals 992ms. This means that we are injecting pan with a velocity of 504
  // (500/0.992) instead of 500. That's why if |duration| is not exactly
  // divisible by interval, we need to round up the |packet_count| to inject at
  // intended rate.
  if (duration % static_cast<long>(interval))
    packet_count++;

  return packet_count;
}

// Injects the touch pointer onto primary monitor.
void InjectPointer(POINTER_TOUCH_INFO* touch_info) {
  // Time when touch input packet is injected.
  static uint64_t injection_time = 0;
  LARGE_INTEGER perf_counter_now = {};
  ::QueryPerformanceCounter(&perf_counter_now);
  if (touch_info->pointerInfo.pointerFlags & POINTER_FLAG_DOWN) {
    injection_time = perf_counter_now.QuadPart;
  } else {
    while (perf_counter_now.QuadPart - injection_time <
           injection_interval_in_qpc_units) {
      ::QueryPerformanceCounter(&perf_counter_now);
    }
    injection_time += injection_interval_in_qpc_units;
  }
  touch_info->pointerInfo.PerformanceCount = injection_time;
  InjectTouchInput(1, touch_info);
}

// Sends a pointer down event to primary monitor.
void SendPointerDown(POINTER_TOUCH_INFO* touch_info) {
  touch_info->pointerInfo.pointerFlags =
      (POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT);
  InjectPointer(touch_info);
}

// Sends a pointer update event to primary monitor.
void SendPointerMove(POINTER_TOUCH_INFO* touch_info) {
  touch_info->pointerInfo.pointerFlags =
      (POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT);
  InjectPointer(touch_info);
}

// Sends a pointer up event to primary monitor.
void SendPointerUp(POINTER_TOUCH_INFO* touch_info) {
  touch_info->pointerInfo.pointerFlags = POINTER_FLAG_UP;
  InjectPointer(touch_info);
}

// Entry point for the application.
int __cdecl main(int argc, const char** argv) {
  // Default values
  // Total number of pixels to move in a direction
  int distance = 500;
  // Number of second to take to perform the pan gesture.
  float duration = 1.0f;
  // Number of times to repeat the input sequence.
  int repeat = 1;
  // Number of seconds to wait before starting any input.
  float start_delay = 1;
  // Number of seconds to wait between each segment of the input sequence.
  float segment_delay = 3;
  // Frequency/frame rate at which input needs to be injected.
  int frequency = 100;
  // True if pan gesture needs to accelerate.
  bool acceleration = false;

  // Parse command line arguments.
  for (int i = 1; i < argc; i++) {
    if (_stricmp(argv[i], "duration") == 0) {
      duration = static_cast<float>(atof(argv[++i]));
    } else if (_stricmp(argv[i], "repeat") == 0) {
      repeat = atoi(argv[++i]);
    } else if (_stricmp(argv[i], "segmentdelay") == 0) {
      segment_delay = static_cast<float>(atof(argv[++i]));
    } else if (_stricmp(argv[i], "startdelay") == 0) {
      start_delay = static_cast<float>(atof(argv[++i]));
    } else if (_stricmp(argv[i], "distance") == 0) {
      distance = atoi(argv[++i]);
    } else if (_stricmp(argv[i], "frequency") == 0) {
      frequency = atoi(argv[++i]);
    } else if (_stricmp(argv[i], "accelerate") == 0) {
      acceleration = true;
    } else {
      printf(
          "pan.exe [repeat n] [startdelay n] [segmentdelay n] [distance n] "
          "[duration n] [frequency n] [accelerate] \r\n");
      printf("\r\n");
      printf(
          "    Note that touch input is injected at  100, 100 + "
          "segmentdistance\r\n");
      printf("\r\n");
      printf(
          "    Example: pan repeat 3 segmentdelay 0.5 distance 100 duration "
          "0.75\r\n");
      printf("\r\n");
      printf(
          "    repeat - number of times to repeat the input sequence. The "
          "default value is 1.\r\n");
      printf(
          "    startdelay - number of seconds to wait before starting any "
          "input. The default value is 1 second.\r\n");
      printf(
          "    segmentdelay - number of seconds to wait between each segment "
          "of the input sequence. The default value is 3 second.\r\n");
      printf(
          "    distance - total number of pixels to move in a direction. The "
          "default value is 500 pixels.\r\n");
      printf("\r\n");
      printf(
          "    duration - number of second to take to perform the pan gesture. "
          "The default value is 1 second.\r\n");
      printf("\r\n");
      printf(
          "    frequency - frequency/frame rate at which input needs to be "
          "injected. The default value is 100 frames per second.\r\n");
      printf(
          "    accelerate - accelerate injection tool instead of linear "
          "movement. The default value is false. \r\n");
      return -1;
    }
  }

  // Increase the thread priority to help ensure we're getting
  // input delivered in a timely manner.
  if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
      printf("Error calling SetPriorityClass: %d\r\n", GetLastError());
      return -1;
  }
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL)) {
      printf("Error calling SetThreadPriority: %d\r\n", GetLastError());
      return -1;
  }

  // Starting point for a pan gesture.
  long startx = 100;
  long starty = 100 + distance;
  // End point for a pan gesture.
  long endy = 100;
  long duration_in_ms = static_cast<long>(duration * MS_PER_SEC);
  double injection_interval_in_ms_from_frequency =
      InjectionIntervalInMsFromFrequency(frequency);

  // Request minimum resolution of |injection_interval_in_ms_from_frequency| ms
  // for timer. Timer can fire either early or late relative to
  // |injection_interval_in_ms_from_frequency|. Without |timeBeginPeriod| being
  // called, this range varies from -2 to 2 ms. |timeBeginPeriod| helps improve
  // timer firing accuracy and reduces the range from -0.7 to 0.6ms.
  timeBeginPeriod(
      static_cast<uint32_t>(injection_interval_in_ms_from_frequency));
  HANDLE hTimer = CreateWaitableTimer(nullptr, FALSE, nullptr);
  if (hTimer == nullptr) {
    printf("Error calling CreateWaitableTimer: %d\r\n", GetLastError());
    return -1;
  }

  LARGE_INTEGER qpc_frequency = {};
  ::QueryPerformanceFrequency(&qpc_frequency);

  injection_interval_in_qpc_units = static_cast<uint64_t>(
      injection_interval_in_ms_from_frequency *
      (MS_PER_SEC * (qpc_frequency.QuadPart / US_PER_SEC)));

  // Create a negative 64-bit integer that will be used to signal the timer 
  // |injection_interval_in_ms_from_frequency| milliseconds from now. Negative 
  // values allows relative time offset instead of absolute times.
  LARGE_INTEGER li_due_time;
  li_due_time.QuadPart =
      -(static_cast<LONGLONG>(injection_interval_in_ms_from_frequency * 10000));

  if (!SetWaitableTimer(
          hTimer, &li_due_time,
          static_cast<long>(injection_interval_in_ms_from_frequency), nullptr,
          nullptr, 0)) {
    printf("Error calling SetWaitableTimer: %d\r\n", GetLastError());
    CloseHandle(hTimer);
    return -1;
  }

  POINTER_TOUCH_INFO contact;
  memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));

  // Initialize touch injection with max of 1 contacts.
  InitializeTouchInjection(1, TOUCH_FEEDBACK_DEFAULT);

  // Delay the start of injection by |start_delay| seconds.
  Sleep(static_cast<DWORD>(start_delay * MS_PER_SEC));

  // Number of pointers to be injected for desired gesture.
  uint32_t packets = CalculatePacketsNeeded(
      duration_in_ms, injection_interval_in_ms_from_frequency);

  for (int c = 0; c < repeat; c++) {
    contact.pointerInfo.pointerType = PT_TOUCH;  // we're sending touch input
    contact.pointerInfo.pointerId = 0;           // contact 0
    contact.pointerInfo.ptPixelLocation.x = startx;
    contact.pointerInfo.ptPixelLocation.y = starty;
    contact.touchFlags = TOUCH_FLAG_NONE;
    contact.touchMask =
        TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
    contact.orientation = 0;
    contact.pressure = 0;
    contact.rcContact.top = contact.pointerInfo.ptPixelLocation.y - 2;
    contact.rcContact.bottom = contact.pointerInfo.ptPixelLocation.y + 2;
    contact.rcContact.left = contact.pointerInfo.ptPixelLocation.x - 2;
    contact.rcContact.right = contact.pointerInfo.ptPixelLocation.x + 2;

    SendPointerDown(&contact);

    for (uint32_t i = 1; i <= packets; i++) {
      if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) {
        printf("WaitForSingleObject failed (%d)\n", GetLastError());
        CancelWaitableTimer(hTimer);
        CloseHandle(hTimer);
        return -1;
      }

      double ratio = static_cast<double>(i) / static_cast<double>(packets);
      if (acceleration)
        ratio *= (1.0f * (ratio - 1) + 1);

      // Updating the Y Co-ordinate
      contact.pointerInfo.ptPixelLocation.y = Interpolate(starty, endy, ratio);
      contact.rcContact.right = contact.pointerInfo.ptPixelLocation.y + 2;
      contact.rcContact.bottom = contact.pointerInfo.ptPixelLocation.y + 2;
      SendPointerMove(&contact);
    }

    SendPointerUp(&contact);

    // Delay the next sequence of injection by |segment_delay| seconds.
    if (repeat != 1)
      Sleep(static_cast<DWORD>(segment_delay * MS_PER_SEC));
  }

  CancelWaitableTimer(hTimer);
  CloseHandle(hTimer);
  timeEndPeriod(static_cast<uint32_t>(injection_interval_in_ms_from_frequency));
  return 0;
}