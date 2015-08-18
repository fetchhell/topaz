//----------------------------------------------------------------------------------
// File:        NvAppBase/NvFramerateCounter.h
// SDK Version: v2.11 
// Email:       gameworks@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2014-2015, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//----------------------------------------------------------------------------------
 
/* Framerate stats */

#ifndef NVFRAMERATECOUNTER_H
#define NVFRAMERATECOUNTER_H

#include <NvFoundation.h>

/// \file
/// Simple mean frame rate timer

class NvStopWatch;
class NvStopWatchFactory;

/// Simple frame rate timer and reporter.
class NvFramerateCounter 
{
public:
    /// Constructor.
    /// \param[in] app the application object to be used to create the required stopwatch
    NvFramerateCounter(NvStopWatchFactory* factory);

    /// Destructor.
    ~NvFramerateCounter();

    /// Frame delimiter.
    /// Call at the end of each frame to mark the end of a frame.
    /// \return true if the frame rate stats have been updated by this call and should
    /// be printed/updated to the user as appropriate
    bool nextFrame();

    /// Reset.
    /// Restarts the counters - should be called when the app has come back
    /// after being paused, etc.  Avoids incorrect stats generated by an
    /// extremely "long" frame (which was actually the app sleeping)
    void reset();

    /// Mean frame rate.
    /// Mean frame rate since last report (the last time nextFrame returned "true")
    /// \return the frame rate in frames per second
    float getMeanFramerate() { return m_meanFramerate; }

    /// Set the report rate in frames.
    /// \param[in] frames the minimum number of frames that must elapse between each
    /// mean frame rate update (i.e. between calls to #nextFrame that return true)
    void setReportFrames(int32_t frames) { m_reportFrames = frames; }

    /// Get the report rate in frames.
    /// \return the minimum number of frames that must elapse between each
    /// mean frame rate update (i.e. between calls to #nextFrame that return true)
    int32_t getReportFrames() { return m_reportFrames; }

    /// Set the report rate in seconds
    /// \param[in] secs the minimum number of seconds of wall-clock time that must elapse between each
    /// mean frame rate update (i.e. between calls to #nextFrame that return true)
    void setMaxReportRate(float secs) { m_reportRate = secs; }

    /// Set the report rate in seconds
    /// \return the minimum number of seconds of wall-clock time that must elapse between each
    /// mean frame rate update (i.e. between calls to #nextFrame that return true)
    float getReportRate() { return m_reportRate; }

protected:
    /// \privatesection
    NvStopWatch* m_stopWatch;
    int32_t m_reportFrames;
    int32_t m_framesSinceReport;
    float m_reportRate;
    float m_meanFramerate;
};

#endif
