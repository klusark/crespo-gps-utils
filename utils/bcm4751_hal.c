// Copyright (C) 2012 Paul Kocialkowski, contact@paulk.fr, GNU GPLv3+
// BCM4751 HAL code
// This code will open the gps lib only: we can trace it that way

#include <stdlib.h>
#include <stdio.h>

#include <hardware/hardware.h>
#include <hardware/gps.h>
#include <hardware_legacy/power.h>

#define WAKE_LOCK_NAME  "GPS"

hw_module_t* module;
hw_device_t* device;

static const GpsInterface* sGpsInterface = NULL;
static const GpsXtraInterface* sGpsXtraInterface = NULL;
static const AGpsInterface* sAGpsInterface = NULL;
static const GpsNiInterface* sGpsNiInterface = NULL;
static const GpsDebugInterface* sGpsDebugInterface = NULL;
static const AGpsRilInterface* sAGpsRilInterface = NULL;

static void location_callback(GpsLocation* location)
{
	printf("got a location callback!\n");
}

static void status_callback(GpsStatus* status)
{
	printf("got a status callback!\n");
}

static void sv_status_callback(GpsSvStatus* sv_status)
{
	printf("got a svstatus callback!\n");
}

static void nmea_callback(GpsUtcTime timestamp, const char* nmea, int length)
{
	printf("got a nmea callback: %s!\n", nmea);
}

static void set_capabilities_callback(uint32_t capabilities)
{
	printf("set_capabilities_callback: %ld\n", capabilities);
}


static void acquire_wakelock_callback()
{
	acquire_wake_lock(PARTIAL_WAKE_LOCK, WAKE_LOCK_NAME);
}

static void release_wakelock_callback()
{
	release_wake_lock(WAKE_LOCK_NAME);
}

static pthread_t create_thread_callback(const char* name, void (*start)(void *), void* arg)
{
	printf("doing a thread!\n");

	pthread_t thread;
	pthread_create(&thread, NULL, start, arg);
	return thread;
}

GpsCallbacks sGpsCallbacks = {
	sizeof(GpsCallbacks),
	location_callback,
	status_callback,
	sv_status_callback,
	nmea_callback,
	set_capabilities_callback,
	acquire_wakelock_callback,
	release_wakelock_callback,
	create_thread_callback,
};

void hal_init(void)
{
	int rc;
	rc = hw_get_module("gps", (hw_module_t const**)&module);
	printf("hw_get_module: %d\n", rc);

	if(rc < 0)
		return;

	rc = module->methods->open(module, GPS_HARDWARE_MODULE_ID, &device);
	printf("module->methods->open: %d\n", rc);

	if(rc < 0)
		return;

	struct gps_device_t* gps_device = (struct gps_device_t *)device;
	sGpsInterface = gps_device->get_gps_interface(gps_device);

	if (sGpsInterface) {
		printf("GPS interface ready\n");
		sGpsXtraInterface =
			(const GpsXtraInterface*)sGpsInterface->get_extension(GPS_XTRA_INTERFACE);
		sAGpsInterface =
			(const AGpsInterface*)sGpsInterface->get_extension(AGPS_INTERFACE);
		sGpsNiInterface =
			(const GpsNiInterface*)sGpsInterface->get_extension(GPS_NI_INTERFACE);
		sGpsDebugInterface =
			(const GpsDebugInterface*)sGpsInterface->get_extension(GPS_DEBUG_INTERFACE);
		sAGpsRilInterface =
			(const AGpsRilInterface*)sGpsInterface->get_extension(AGPS_RIL_INTERFACE);
	}
}

void gps_init(void)
{
	int rc;
	rc = sGpsInterface->init(&sGpsCallbacks);
	printf("sGpsInterface->init: %d\n", rc);
	
/*
	// if XTRA initialization fails we will disable it by sGpsXtraInterface to null,
	// but continue to allow the rest of the GPS interface to work.
	if (sGpsXtraInterface && sGpsXtraInterface->init(&sGpsXtraCallbacks) != 0)
		sGpsXtraInterface = NULL;
	if (sAGpsInterface)
		sAGpsInterface->init(&sAGpsCallbacks);
	if (sGpsNiInterface)
		sGpsNiInterface->init(&sGpsNiCallbacks);
	if (sAGpsRilInterface)
		sAGpsRilInterface->init(&sAGpsRilCallbacks);
*/
}

int main(void)
{
	hal_init();
	gps_init();

	sleep(1);

	printf("Starting GPS!\n");
	sGpsInterface->start();
	sleep(60);
	printf("Stopping GPS!\n");
	sGpsInterface->stop();

	sleep(60);

	return 0;
}
