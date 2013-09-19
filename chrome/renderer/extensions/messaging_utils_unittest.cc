// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/stringprintf.h"
#include "chrome/test/base/module_system_test.h"
#include "grit/renderer_resources.h"

namespace extensions {
namespace {

class MessagingUtilsUnittest : public ModuleSystemTest {
 protected:
  void RegisterTestModule(const char* code) {
    RegisterModule("test", base::StringPrintf(
        "var assert = requireNative('assert');\n"
        "var AssertTrue = assert.AssertTrue;\n"
        "var AssertFalse = assert.AssertFalse;\n"
        "var messagingUtils = require('messaging_utils');\n"
        "%s", code));
  }

 private:
  virtual void SetUp() OVERRIDE {
    ModuleSystemTest::SetUp();

    RegisterModule("messaging_utils", IDR_MESSAGING_UTILS_JS);
  }

};

TEST_F(MessagingUtilsUnittest, TestNothing) {
  ExpectNoAssertionsMade();
}

TEST_F(MessagingUtilsUnittest, NoArguments) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments();\n"
      "AssertTrue(args === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, ZeroArguments) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments([]);"
      "AssertTrue(args === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, TooManyArguments) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    ['a', 'b', 'c', 'd']);\n"
      "AssertTrue(args === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, FinalArgumentIsNotAFunction) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(\n"
      "    ['a', 'b', 'c']);\n"
      "AssertTrue(args === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  // Because the request argument is required, a single argument must get
  // mapped to it rather than to the optional targetId argument.
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(['a']);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == 'a');\n"
      "AssertTrue(args[2] === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringAndOneNullArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  // Explicitly specifying null as the request is allowed.
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments(['a', null]);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] == 'a');\n"
      "AssertTrue(args[1] === null);\n"
      "AssertTrue(args[2] === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneNullAndOneStringArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  RegisterTestModule(
      "var args = messagingUtils.alignSendMessageArguments([null, 'a']);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == 'a');\n"
      "AssertTrue(args[2] === null);");
  module_system_->Require("test");
}

TEST_F(MessagingUtilsUnittest, OneStringAndOneFunctionArgument) {
  ModuleSystem::NativesEnabledScope natives_enabled_scope(module_system_.get());
  // When the arguments are a string and a function, the function is
  // unambiguously the responseCallback. Because the request argument is
  // required, the remaining argument must get mapped to it rather than to the
  // optional targetId argument.
  RegisterTestModule(
      "var cb = function() {};\n"
      "var args = messagingUtils.alignSendMessageArguments(['a', cb]);\n"
      "AssertTrue(args.length == 3);\n"
      "AssertTrue(args[0] === null);\n"
      "AssertTrue(args[1] == 'a');\n"
      "AssertTrue(args[2] == cb);");
  module_system_->Require("test");
}

}  // namespace
}  // namespace extensions
