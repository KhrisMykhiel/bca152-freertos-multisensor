#pragma once
/*
 * rtos_objects.h
 *
 * Central declaration point for every FreeRTOS IPC/synchronization object
 * shared across tasks: the sensor queue, the serial-output mutex, and the
 * system event group. Defined once in rtos_objects.cpp, created in
 * main.cpp, referenced by every task module.
 *
 * WHY CENTRALIZE THIS: without a single header, task modules would need to
 * pass handles around manually or duplicate extern declarations. This also
 * makes the "what talks to what" architecture auditable in one place.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_group.h"
#include "freertos/task.h"

// ---- Queues -----------------------------------------------------------
// A plain FreeRTOS queue is single-consumer: one xQueueReceive() call
// removes the item, so it cannot fan the same sample out to two
// independent tasks. SensorTask therefore posts each reading onto TWO
// queues, one per consumer, matching the SensorTask -> {DisplayTask,
// AlarmTask} branching shown in the lab's architecture diagram.
extern QueueHandle_t sensorQueueDisplay;
extern QueueHandle_t sensorQueueAlarm;
#define SENSOR_QUEUE_LENGTH 5

// ---- Mutex -------------------------------------------------------------
// Protects shared Serial (UART/printf) output so diagnostic lines from
// different tasks are never interleaved mid-line.
extern SemaphoreHandle_t serialMutex;

// ---- Event group ---------------------------------------------------
// Bits represent system-level events any task can observe.
extern EventGroupHandle_t systemEventGroup;
#define EVENT_ACTIVE   (1 << 0)  // set = system is ACTIVE, cleared = INACTIVE
#define EVENT_MOTION   (1 << 1)  // pulsed by MotionTask on each detected motion
#define EVENT_ALARM    (1 << 2)  // set while AlarmState != NORMAL

// ---- Task priorities (see docs/laboratory-report for justification) ----
#define PRIORITY_MOTION   3
#define PRIORITY_INPUT    3
#define PRIORITY_SENSOR   2
#define PRIORITY_ALARM    2
#define PRIORITY_STATE    2
#define PRIORITY_DISPLAY  1

// ---- Task stack sizes ----
#define STACK_SENSOR   4096
#define STACK_DISPLAY  4096
#define STACK_INPUT    3072
#define STACK_MOTION   2048
#define STACK_ALARM    2048
#define STACK_STATE    2048

// ---- Timing ----
#define SENSOR_PERIOD_MS        2000
#define INACTIVITY_TIMEOUT_MS   15000
#define LOW_TEMPERATURE_C       18.0f
#define HIGH_TEMPERATURE_C      30.0f

// Creates the queue, mutex, and event group. Must run before any task
// that touches them is created (called early in app_main).
void rtosObjectsInit();

// Helper: mutex-protected printf, used by every task for diagnostics so
// output on the Serial monitor is never interleaved.
void safePrintf(const char *fmt, ...);
