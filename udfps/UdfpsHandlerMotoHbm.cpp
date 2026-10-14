/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.moto"

#include <aidl/android/hardware/biometrics/fingerprint/AcquiredInfo.h>

#include <chrono>
#include <mutex>
#include <thread>

#include "UdfpsHandlerMoto.h"

#ifndef SCREEN_WAKE_DELAY
#define SCREEN_WAKE_DELAY 0
#endif

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

class MotoUdfpsHandlerHbm : public UdfpsHandler {
  public:
    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) override {
        if (SCREEN_WAKE_DELAY > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SCREEN_WAKE_DELAY));
        }
        enableHighBrightFod();
    }

    void onFingerUp() override { disableHighBrightFod(); }

    void onAcquired(int32_t result, int32_t /*vendorCode*/) override {
        // The engines hand us the AIDL AcquiredInfo value.
        if (result == static_cast<int32_t>(AcquiredInfo::GOOD)) {
            // Set finger as up to disable HBM already, even if the finger is still pressed
            onFingerUp();
        }
    }

    void onAuthenticationSucceeded() override { disableHighBrightFod(); }
    void onAuthenticationFailed() override { disableHighBrightFod(); }
    void cancel() override { disableHighBrightFod(); }

  private:
    void disableHighBrightFod() {
        std::lock_guard<std::mutex> lock(mSetHbmFodMutex);

        if (!mHbmFodEnabled) {
            return;
        }

        setHbmState(OFF);

        mHbmFodEnabled = false;
    }

    void enableHighBrightFod() {
        std::lock_guard<std::mutex> lock(mSetHbmFodMutex);

        if (mHbmFodEnabled) {
            return;
        }

        setHbmState(ON);

        mHbmFodEnabled = true;
    }

    bool mHbmFodEnabled = false;
    std::mutex mSetHbmFodMutex;
};

static UdfpsHandler* create() {
    return new MotoUdfpsHandlerHbm();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};
