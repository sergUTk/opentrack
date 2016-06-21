/* Copyright (c) 2014 Donovan Baarda <abo@minkirri.apana.org.au>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
#include "ftnoir_filter_ewma2.h"
#include <cmath>
#include <QDebug>
#include <QWidget>
#include "opentrack/plugin-api.hpp"
#include <algorithm>
#include <QMutexLocker>

// Exponentially Weighted Moving Average (EWMA) Filter with dynamic smoothing.
//
// This filter tries to adjust the amount of filtering to minimize lag when
// moving, and minimize noise when still. It uses the delta filtered over the
// last 1/60sec (16ms) compared to the delta's average noise variance over
// the last 60sec to try and detect movement vs noise. As the delta increases
// from 0 to 3 stdevs of the noise, the filtering scales down from maxSmooth
// to minSmooth at a rate controlled by the powCurve setting.


FTNoIR_Filter::FTNoIR_Filter() : first_run(true)
{
}

void FTNoIR_Filter::filter(const double *input, double *output)
{
    // Start the timer and initialise filter state if it's not running.
    if (first_run)
    {
        first_run = false;
        sample_timer.start();
        filter_timer.start();
        for (unsigned i = 0; i < 6; i++)
        {
            last_output[i] = input[i];
            last_delta[i] = 0;
            last_noise[i] = 0;
            dt_ = 0;
        }
    }

    bool new_sample = false;

    for (unsigned i = 0; i < 6; i++)
        if (fabs(last_output[i] - input[i]) > 1e-4)
        {
            new_sample = true;
            break;
        }

    // Get the time in seconds since last run and restart the timer.

    const double dt_s = new_sample ? sample_timer.elapsed_seconds() : 0;
    const double dt_f = filter_timer.elapsed_seconds();

    if (new_sample)
        sample_timer.start();
    filter_timer.start();

    // Calculate delta_alpha and noise_alpha from dt.
    double delta_alpha = dt_s/(dt_s + delta_RC);
    double noise_alpha = dt_s/(dt_s + noise_RC);

    // scale curve .01->1 where 1.0 is sqrt(norm_noise).
    const double smoothing_scale_curve = static_cast<slider_value>(s.kSmoothingScaleCurve);
    // min/max smoothing .01->1
    const double min_smoothing = static_cast<slider_value>(s.kMinSmoothing);
    const double max_smoothing = std::max(min_smoothing, static_cast<slider_value>(s.kMaxSmoothing).cur());

    // Calculate the new camera position.
    for (unsigned i = 0; i < 6; i++)
    {
        using std::pow;
        using std::fabs;

        // Calculate the current and smoothed delta.
        double delta = input[i] - last_output[i];
        last_delta[i] = delta_alpha*delta + (1.0-delta_alpha)*last_delta[i];
        // Calculate the current and smoothed noise variance.
        double noise = last_delta[i]*last_delta[i];
        last_noise[i] = noise_alpha*noise + (1.0-noise_alpha)*last_noise[i];
        // Normalise the noise between 0->1 for 0->9 variances (0->3 stddevs).
        double norm_noise = last_noise[i] < 1e-10 ? 0 : std::min<double>(noise/(9.0*last_noise[i]), 1.0);

        static const double c[] = { 5, 5, 3, 1, 1, 1 };

        // Calculate the smoothing 0.0->1.0 from the normalized noise.
        double smoothing = 1.0 - pow(norm_noise, smoothing_scale_curve);
        double RC = c[i] * (min_smoothing + smoothing*(max_smoothing - min_smoothing));
        // Calculate the dynamic alpha.
        double alpha = dt_f/(dt_f + RC);
        // Calculate the new output position.
        output[i] = last_output[i] = alpha*input[i] + (1.0-alpha)*last_output[i];
    }
}

OPENTRACK_DECLARE_FILTER(FTNoIR_Filter, FilterControls, FTNoIR_FilterDll)
