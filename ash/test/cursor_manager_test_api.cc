// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test/cursor_manager_test_api.h"

#include "ash/shell.h"
#include "ash/test/shell_test_api.h"
#include "ash/wm/ash_native_cursor_manager.h"
#include "ash/wm/image_cursors.h"
#include "ui/gfx/display.h"
#include "ui/views/corewm/cursor_manager.h"

namespace ash {
namespace test {

CursorManagerTestApi::CursorManagerTestApi(
    views::corewm::CursorManager* cursor_manager)
    : cursor_manager_(cursor_manager) {
}

CursorManagerTestApi::~CursorManagerTestApi() {
}

// TODO(tdanderson): CursorManagerTestApi may no longer be needed.
float CursorManagerTestApi::GetCurrentScale() const {
  return cursor_manager_->GetScale();
}

ui::CursorSetType CursorManagerTestApi::GetCurrentCursorSet() const {
  return cursor_manager_->GetCursorSet();
}

gfx::NativeCursor CursorManagerTestApi::GetCurrentCursor() const {
  return cursor_manager_->GetCursor();
}

gfx::Display CursorManagerTestApi::GetDisplay() const {
  return ShellTestApi(Shell::GetInstance()).ash_native_cursor_manager()->
      image_cursors_->GetDisplay();
}

}  // namespace test
}  // namespace ash
