/* Ticker library code is placed under the MIT license
 * Copyright (c) 2018 Stefan Staub
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TICKERFREE_H
#define TICKERFREE_H

#include "Arduino.h"

/** Ticker internal resolution
 *
 * @param MICROS interval argument in ms, internal timing in µs (max ~70 min)
 * @param MILLIS interval in milliseconds (longer periods)
 * @param MICROS_MICROS interval and internal timing in microseconds
 */
enum resolution_t { MICROS, MILLIS, MICROS_MICROS };

/** Ticker status */
enum status_t { STOPPED, RUNNING, PAUSED };

#if defined(__arm__) || defined(ESP8266) || defined(ESP32)
#include <functional>
using CallbackType = std::function<void()>;
#else
typedef void (*CallbackType)();
#endif

template <typename... Args> class TickerFree {
  protected:
	using CallbackType = std::function<void(Args...)>;
	CallbackType callback;

  public:
	TickerFree(CallbackType callback, uint32_t timer, uint32_t repeat = 0,
			   resolution_t resolution = MICROS)
		: callback(callback),
		  enabled(false),
		  timer((resolution == MICROS) ? (timer * 1000) : timer),
		  repeat(repeat), resolution(resolution), counts(0), status(STOPPED),
		  lastTime(0), diffTime(0) {}

	~TickerFree() {}

	void start() {
		if (!callback)
			return;
		if (resolution == MILLIS)
			lastTime = millis();
		else
			lastTime = micros();
		enabled = true;
		counts = 0;
		status = RUNNING;
	}

	void resume() {
		if (!callback)
			return;
		if (resolution == MILLIS)
			lastTime = millis() - diffTime;
		else
			lastTime = micros() - diffTime;
		if (status == STOPPED)
			counts = 0;
		enabled = true;
		status = RUNNING;
	}

	void pause() {
		if (resolution == MILLIS)
			diffTime = millis() - lastTime;
		else
			diffTime = micros() - lastTime;
		enabled = false;
		status = PAUSED;
	}

	void stop() {
		enabled = false;
		counts = 0;
		status = STOPPED;
	}

	void update() {
		if (tick() && callback)
			callback();
	}

	void trigger(Args... args) {
		if (tick() && callback)
			callback(args...);
	}

	void interval(uint32_t t) {
		if (resolution == MICROS)
			t *= 1000;
		timer = t;
	}

	/** Interval in user units: ms for MICROS/MILLIS, µs for MICROS_MICROS */
	uint32_t interval() const {
		if (resolution == MICROS)
			return timer / 1000;
		return timer;
	}

	uint32_t elapsed() const {
		if (resolution == MILLIS)
			return millis() - lastTime;
		return micros() - lastTime;
	}

	uint32_t remaining() const {
		uint32_t e = elapsed();
		return (e >= timer) ? 0 : (timer - e);
	}

	status_t state() const { return status; }

	uint32_t counter() const { return counts; }

	void setCallback(CallbackType cb) { callback = cb; }

	void repeats(uint32_t r) { repeat = r; }

  private:
	bool tick() {
		if (!enabled)
			return false;
		uint32_t currentTime = (resolution == MILLIS) ? millis() : micros();
		if ((currentTime - lastTime) >= timer) {
			lastTime = currentTime;
			if (repeat - counts == 1 && counts != 0xFFFFFFFF) {
				enabled = false;
				status = STOPPED;
			}
			counts++;
			return true;
		}
		return false;
	}

	bool enabled;
	uint32_t timer;
	uint32_t repeat;
	resolution_t resolution;
	uint32_t counts;
	status_t status;
	uint32_t lastTime;
	uint32_t diffTime;
};

template <> class TickerFree<> {
  protected:
	using CallbackType = std::function<void()>;
	CallbackType callback;

  public:
	TickerFree(CallbackType callback, uint32_t timer, uint32_t repeat = 0,
			   resolution_t resolution = MICROS)
		: callback(callback),
		  timer((resolution == MICROS) ? (timer * 1000) : timer),
		  repeat(repeat), resolution(resolution), enabled(false), lastTime(0),
		  counts(0), status(STOPPED), diffTime(0) {}

	~TickerFree() {}

	void start() {
		if (!callback)
			return;
		if (resolution == MILLIS)
			lastTime = millis();
		else
			lastTime = micros();
		enabled = true;
		counts = 0;
		status = RUNNING;
	}

	void resume() {
		if (!callback)
			return;
		if (resolution == MILLIS)
			lastTime = millis() - diffTime;
		else
			lastTime = micros() - diffTime;
		if (status == STOPPED)
			counts = 0;
		enabled = true;
		status = RUNNING;
	}

	void pause() {
		if (resolution == MILLIS)
			diffTime = millis() - lastTime;
		else
			diffTime = micros() - lastTime;
		enabled = false;
		status = PAUSED;
	}

	void stop() {
		enabled = false;
		counts = 0;
		status = STOPPED;
	}

	void update() {
		if (tick() && callback)
			callback();
	}

	void interval(uint32_t t) {
		if (resolution == MICROS)
			t *= 1000;
		timer = t;
	}

	uint32_t interval() const {
		if (resolution == MICROS)
			return timer / 1000;
		return timer;
	}

	uint32_t elapsed() const {
		if (resolution == MILLIS)
			return millis() - lastTime;
		return micros() - lastTime;
	}

	uint32_t remaining() const {
		uint32_t e = elapsed();
		return (e >= timer) ? 0 : (timer - e);
	}

	status_t state() const { return status; }

	uint32_t counter() const { return counts; }

	void setCallback(CallbackType cb) { callback = cb; }

	void repeats(uint32_t r) { repeat = r; }

  private:
	bool tick() {
		if (!enabled)
			return false;
		uint32_t currentTime = (resolution == MILLIS) ? millis() : micros();
		if ((currentTime - lastTime) >= timer) {
			lastTime = currentTime;
			if (repeat - counts == 1 && counts != 0xFFFFFFFF) {
				enabled = false;
				status = STOPPED;
			}
			counts++;
			return true;
		}
		return false;
	}

	bool enabled;
	uint32_t timer;
	uint32_t repeat;
	resolution_t resolution;
	uint32_t counts;
	status_t status;
	uint32_t lastTime;
	uint32_t diffTime;
};

#endif
