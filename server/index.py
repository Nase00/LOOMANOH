# -*- coding: utf-8 -*-
"""
Adapted from https://github.com/NeuroTechX/bci-workshop
"""

import math
import threading
import sys
import time
import requests

# Original muselsl deps
import numpy as np  # Module that simplifies computations on matrices
import matplotlib.pyplot as plt  # Module used for plotting
from pylsl import StreamInlet, resolve_byprop  # Module to receive EEG data
import utils  # Our own utility functions

# Phat deps
import microdotphat

ENDPOINT = "http://10.4.53.51:3000/move"
BASELINE_BOUNDS = 0.04
NEW_BASELINE_THRESHOLD = 0.05
baseline = 0

# standing
# UPPER_BOUNDS = 0.8
# LOWER_THRESHOLD = 0.72

# alpha_metric_rounded = 0
phat_text = "LOOMANOH"

# Handy little enum to make code more readable


class Band:
    Delta = 0
    Theta = 1
    Alpha = 2
    Beta = 3


""" EXPERIMENTAL PARAMETERS """
# Modify these to change aspects of the signal processing

# Length of the EEG data buffer (in seconds)
# This buffer will hold last n seconds of data and be used for calculations
BUFFER_LENGTH = 5

# Length of the epochs used to compute the FFT (in seconds)
EPOCH_LENGTH = 1

# Amount of overlap between two consecutive epochs (in seconds)
OVERLAP_LENGTH = 0.8

# Amount to 'shift' the start of each next consecutive epoch
SHIFT_LENGTH = EPOCH_LENGTH - OVERLAP_LENGTH

# Index of the channel(s) (electrodes) to be used
# 0 = left ear, 1 = left forehead, 2 = right forehead, 3 = right ear
INDEX_CHANNEL = [0]


def stream():
    global baseline

    """ 1. CONNECT TO EEG STREAM """

    # Search for active LSL streams
    print('Looking for an EEG stream...')
    streams = resolve_byprop('type', 'EEG', timeout=2)
    if len(streams) == 0:
        raise RuntimeError('Can\'t find EEG stream.')

    # Set active EEG stream to inlet and apply time correction
    print("Start acquiring data")
    inlet = StreamInlet(streams[0], max_chunklen=12)
    eeg_time_correction = inlet.time_correction()

    # Get the stream info and description
    info = inlet.info()
    description = info.desc()

    # Get the sampling frequency
    # This is an important value that represents how many EEG data points are
    # collected in a second. This influences our frequency band calculation.
    # for the Muse 2016, this should always be 256
    fs = int(info.nominal_srate())

    """ 2. INITIALIZE BUFFERS """

    # Initialize raw EEG data buffer
    eeg_buffer = np.zeros((int(fs * BUFFER_LENGTH), 1))
    filter_state = None  # for use with the notch filter

    # Compute the number of epochs in "buffer_length"
    n_win_test = int(np.floor((BUFFER_LENGTH - EPOCH_LENGTH) /
                              SHIFT_LENGTH + 1))

    # Initialize the band power buffer (for plotting)
    # bands will be ordered: [delta, theta, alpha, beta]
    band_buffer = np.zeros((n_win_test, 4))

    """ 3. GET DATA """

    # The following loop acquires data, computes band powers, and calculates neurofeedback metrics based on those band powers
    while True:
        try:
            time.sleep(0.05)

            """ 3.1 ACQUIRE DATA """
            # Obtain EEG data from the LSL stream
            eeg_data, timestamp = inlet.pull_chunk(
                timeout=1, max_samples=int(SHIFT_LENGTH * fs))

            # Only keep the channel we're interested in
            ch_data = np.array(eeg_data)[:, INDEX_CHANNEL]

            # Update EEG buffer with the new data
            eeg_buffer, filter_state = utils.update_buffer(
                eeg_buffer, ch_data, notch=True,
                filter_state=filter_state)

            """ 3.2 COMPUTE BAND POWERS """
            # Get newest samples from the buffer
            data_epoch = utils.get_last_data(eeg_buffer,
                                             EPOCH_LENGTH * fs)

            # Compute band powers
            band_powers = utils.compute_band_powers(data_epoch, fs)
            band_buffer, _ = utils.update_buffer(band_buffer,
                                                 np.asarray([band_powers]))
            # Compute the average band powers for all epochs in buffer
            # This helps to smooth out noise
            smooth_band_powers = np.mean(band_buffer, axis=0)

            # print('Delta: ', band_powers[Band.Delta], ' Theta: ', band_powers[Band.Theta],
            #       ' Alpha: ', band_powers[Band.Alpha], ' Beta: ', band_powers[Band.Beta])

            """ 3.3 COMPUTE NEUROFEEDBACK METRICS """
            # These metrics could also be used to drive brain-computer interfaces

            # Alpha Protocol:
            # Simple redout of alpha power, divided by delta waves in order to rule out noise
            alpha_metric = smooth_band_powers[Band.Alpha] / \
                smooth_band_powers[Band.Delta]
            print('Alpha Relaxation: ', alpha_metric)

            if (baseline == 0):
                baseline = alpha_metric

            if (baseline > alpha_metric + NEW_BASELINE_THRESHOLD or baseline < alpha_metric - NEW_BASELINE_THRESHOLD):
                baseline = alpha_metric

            phat_text = str(alpha_metric)[0:6]
            # microdotphat.write_string(phat_text, kerning=False)
            # microdotphat.show()

            handleMetric(alpha_metric)

            # alpha_metric_rounded = round(alpha_metric * 100, -1)
            # print('Alpha alpha_metric_rounded: ', alpha_metric_rounded)

            # Beta Protocol:
            # Beta waves have been used as a measure of mental activity and concentration
            # This beta over theta ratio is commonly used as neurofeedback for ADHD
            # beta_metric = smooth_band_powers[Band.Beta] / \
            #     smooth_band_powers[Band.Theta]
            # print('Beta Concentration: ', beta_metric)

            # Alpha/Theta Protocol:
            # This is another popular neurofeedback metric for stress reduction
            # Higher theta over alpha is supposedly associated with reduced anxiety
            # theta_metric = smooth_band_powers[Band.Theta] / \
            #     smooth_band_powers[Band.Alpha]
            # print('Theta Relaxation: ', theta_metric)
        except KeyboardInterrupt:
            print("Stopping stream read")
            raise


def handleMetric(value):
    global baseline

    try:
        UPPER_BOUNDS = baseline + BASELINE_BOUNDS
        LOWER_BOUNDS = baseline - BASELINE_BOUNDS

        if (value > UPPER_BOUNDS):
            time.sleep(5)
            print('Baseline: ' + str(baseline))
            print('Value over: ' + str(UPPER_BOUNDS))
            headers = {
                "ms": "5000",
                "direction": "UP"
            }
            requests.post(ENDPOINT, {}, headers=headers)
        elif (value < LOWER_BOUNDS):
            time.sleep(5)
            print('Baseline: ' + str(baseline))
            print('Value under: ' + str(LOWER_BOUNDS))
            headers = {
                "ms": "5000",
                "direction": "DOWN"
            }
            requests.post(ENDPOINT, {}, headers=headers)
    except:
        e = sys.exc_info()[0]
        print("<p>Error: %s</p>" % e)


def setup():
    microdotphat.clear()
    # microdotphat.write_string(phat_text, kerning=False)


def phatText():
    start = time.time()
    speed = 5

    while True:
                # Fade the brightness in/out using a sine wave
        b = (math.sin((time.time() - start) * speed) + 1) / 2
        microdotphat.set_brightness(b)

        # At minimum brightness, swap out the string for the next one
        if b < 0.002 and shown:
            microdotphat.clear()
            microdotphat.write_string(phat_text, kerning=False)

            microdotphat.show()
            shown = False

        # At maximum brightness, confirm the string has been shown
        if b > 0.998:
            shown = True

        # Sleep a bit to save resources, this wont affect the fading speed
        time.sleep(0.01)


def phat():
    while True:
        microdotphat.clear()
        t = time.time() * 10
        for x in range(45):
            y = int((math.sin(t + (x/2.5)) + 1) * 3.5)
            microdotphat.set_pixel(x, y, 1)

        microdotphat.show()
        time.sleep(0.01)


def run():
    tasks = [phat, stream]
    for task in tasks:
        thread = threading.Thread(target=task)
        thread.daemon = False
        thread.start()


if __name__ == '__main__':
    try:
        setup()
        run()

    except KeyboardInterrupt:
        sys.exit()
        pass
