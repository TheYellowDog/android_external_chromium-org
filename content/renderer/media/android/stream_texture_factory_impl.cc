// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/android/stream_texture_factory_impl.h"

#include "cc/output/context_provider.h"
#include "content/common/gpu/client/gpu_channel_host.h"
#include "content/common/gpu/gpu_messages.h"
#include "content/renderer/gpu/stream_texture_host_android.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "ui/gfx/size.h"

namespace content {

namespace {

class StreamTextureProxyImpl : public StreamTextureProxy,
                               public StreamTextureHost::Listener {
 public:
  explicit StreamTextureProxyImpl(StreamTextureHost* host);
  virtual ~StreamTextureProxyImpl();

  // StreamTextureProxy implementation:
  virtual void BindToLoop(int32 stream_id,
                          cc::VideoFrameProvider::Client* client,
                          scoped_refptr<base::MessageLoopProxy> loop) OVERRIDE;
  virtual void Release() OVERRIDE;

  // StreamTextureHost::Listener implementation:
  virtual void OnFrameAvailable() OVERRIDE;
  virtual void OnMatrixChanged(const float matrix[16]) OVERRIDE;

 private:
  void BindOnThread(int32 stream_id);

  const scoped_ptr<StreamTextureHost> host_;

  // Protects access to |client_| and |loop_|.
  base::Lock lock_;
  cc::VideoFrameProvider::Client* client_;
  scoped_refptr<base::MessageLoopProxy> loop_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureProxyImpl);
};

StreamTextureProxyImpl::StreamTextureProxyImpl(StreamTextureHost* host)
    : host_(host), client_(NULL) {}

StreamTextureProxyImpl::~StreamTextureProxyImpl() {}

void StreamTextureProxyImpl::Release() {
  {
    // Cannot call into |client_| anymore (from any thread) after returning
    // from here.
    base::AutoLock lock(lock_);
    client_ = NULL;
  }
  // Release is analogous to the destructor, so there should be no more external
  // calls to this object in Release. Therefore there is no need to acquire the
  // lock to access |loop_|.
  if (!loop_.get() || loop_->BelongsToCurrentThread() ||
      !loop_->DeleteSoon(FROM_HERE, this)) {
    delete this;
  }
}

void StreamTextureProxyImpl::BindToLoop(
    int32 stream_id,
    cc::VideoFrameProvider::Client* client,
    scoped_refptr<base::MessageLoopProxy> loop) {
  DCHECK(loop);

  {
    base::AutoLock lock(lock_);
    DCHECK(!loop_ || (loop == loop_));
    loop_ = loop;
    client_ = client;
  }

  if (loop->BelongsToCurrentThread()) {
    BindOnThread(stream_id);
    return;
  }
  // Unretained is safe here only because the object is deleted on |loop_|
  // thread.
  loop->PostTask(FROM_HERE,
                 base::Bind(&StreamTextureProxyImpl::BindOnThread,
                            base::Unretained(this),
                            stream_id));
}

void StreamTextureProxyImpl::BindOnThread(int32 stream_id) {
  host_->BindToCurrentThread(stream_id, this);
}

void StreamTextureProxyImpl::OnFrameAvailable() {
  base::AutoLock lock(lock_);
  if (client_)
    client_->DidReceiveFrame();
}

void StreamTextureProxyImpl::OnMatrixChanged(const float matrix[16]) {
  base::AutoLock lock(lock_);
  if (client_)
    client_->DidUpdateMatrix(matrix);
}

}  // namespace

// static
scoped_refptr<StreamTextureFactoryImpl> StreamTextureFactoryImpl::Create(
    const scoped_refptr<cc::ContextProvider>& context_provider,
    GpuChannelHost* channel,
    int frame_id) {
  return new StreamTextureFactoryImpl(context_provider, channel, frame_id);
}

StreamTextureFactoryImpl::StreamTextureFactoryImpl(
    const scoped_refptr<cc::ContextProvider>& context_provider,
    GpuChannelHost* channel,
    int frame_id)
    : context_provider_(context_provider),
      channel_(channel),
      frame_id_(frame_id) {
  DCHECK(channel);
}

StreamTextureFactoryImpl::~StreamTextureFactoryImpl() {}

StreamTextureProxy* StreamTextureFactoryImpl::CreateProxy() {
  DCHECK(channel_.get());
  StreamTextureHost* host = new StreamTextureHost(channel_.get());
  return new StreamTextureProxyImpl(host);
}

void StreamTextureFactoryImpl::EstablishPeer(int32 stream_id, int player_id) {
  DCHECK(channel_.get());
  channel_->Send(
      new GpuStreamTextureMsg_EstablishPeer(stream_id, frame_id_, player_id));
}

unsigned StreamTextureFactoryImpl::CreateStreamTexture(
    unsigned texture_target,
    unsigned* texture_id,
    gpu::Mailbox* texture_mailbox) {
  GLuint stream_id = 0;
  gpu::gles2::GLES2Interface* gl = context_provider_->ContextGL();
  gl->GenTextures(1, texture_id);

  stream_id = gl->CreateStreamTextureCHROMIUM(*texture_id);

  gl->GenMailboxCHROMIUM(texture_mailbox->name);
  gl->ProduceTextureDirectCHROMIUM(
      *texture_id, texture_target, texture_mailbox->name);
  return stream_id;
}

void StreamTextureFactoryImpl::SetStreamTextureSize(int32 stream_id,
                                                    const gfx::Size& size) {
  channel_->Send(new GpuStreamTextureMsg_SetSize(stream_id, size));
}

gpu::gles2::GLES2Interface* StreamTextureFactoryImpl::ContextGL() {
  return context_provider_->ContextGL();
}

void StreamTextureFactoryImpl::AddObserver(
    StreamTextureFactoryContextObserver* obs) {
}

void StreamTextureFactoryImpl::RemoveObserver(
    StreamTextureFactoryContextObserver* obs) {
}

}  // namespace content
