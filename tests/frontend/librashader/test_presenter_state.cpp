/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cassert>

#include "librashader/native_presenter.h"
#include "librashader/presenter_factory.h"

using namespace vaeg::librashader;

int main() {
	assert(presenter_state_transition_allowed(PresenterState::Unavailable,
	                                           PresenterState::Initializing));
	assert(presenter_state_transition_allowed(PresenterState::Initializing,
	                                           PresenterState::PassThrough));
	assert(presenter_state_transition_allowed(PresenterState::PassThrough,
	                                           PresenterState::Filtered));
	assert(presenter_state_transition_allowed(PresenterState::Filtered,
	                                           PresenterState::PassThrough));
	assert(!presenter_state_transition_allowed(PresenterState::Unavailable,
	                                            PresenterState::Filtered));
	auto presenter = create_native_presenter(PresenterBackend::Automatic);
	assert(presenter != nullptr);
	assert(presenter->state() == PresenterState::Unavailable);
	assert(presenter->initialize({nullptr, 640, 400, PresenterBackend::Automatic, false, nullptr}) ==
	       PresenterResult::Fallback);
	assert(presenter->last_error() == PresenterError::PlatformUnavailable);
	assert(presenter->resize(1280, 800) == PresenterResult::Fallback);
	return 0;
}
