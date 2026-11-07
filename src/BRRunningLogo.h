#pragma once

#include <FFGLSDK.h>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class BRRunningLogo final : public CFFGLPlugin {
public:
  BRRunningLogo();
  ~BRRunningLogo() override;

  FFResult InitGL(const FFGLViewportStruct* viewport) override;
  FFResult ProcessOpenGL(ProcessOpenGLStruct* process) override;
  FFResult DeInitGL() override;
  FFResult SetFloatParameter(unsigned int index, float value) override;
  float GetFloatParameter(unsigned int index) override;
  FFResult SetTextParameter(unsigned int index, const char* value) override;
  char* GetTextParameter(unsigned int index) override;

  static LRESULT CALLBACK ManagerWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

private:
  void addOption(unsigned int parameter, unsigned int element, const char* name, float value);
  void openLogoManager();
  void runLogoManager();
  void rebuildAtlas();
  void releaseAtlas();
  void setLogoPaths(const std::vector<std::string>& paths);
  std::vector<std::string> getLogoPaths();
  std::string serializePaths(const std::vector<std::string>& paths) const;
  std::vector<std::string> parsePaths(const std::string& value) const;

  std::vector<float> values_;
  std::vector<std::string> logoPaths_;
  std::string serializedPaths_;
  const std::string credit_ = "Effect by Belajar Resolume";
  std::mutex logoMutex_;

  unsigned int viewportWidth_ = 1920;
  unsigned int viewportHeight_ = 1080;
  unsigned int atlasWidth_ = 1;
  unsigned int atlasHeight_ = 1;
  unsigned int atlasTexture_ = 0;
  std::atomic<bool> atlasDirty_{true};
  std::atomic<bool> pathsEventPending_{false};

  unsigned long long gdiplusToken_ = 0;
  std::chrono::steady_clock::time_point lastFrame_;
  double elapsed_ = 0.0;
  double outStartedAt_ = 0.0;
  bool paused_ = false;
  bool visible_ = true;
  bool playingOut_ = false;

  std::atomic<HWND> managerWindow_{nullptr};
  HWND managerList_ = nullptr;
  std::atomic<bool> managerRunning_{false};
  std::atomic<bool> managerStopRequested_{false};
  std::thread managerThread_;

  ffglex::FFGLShader shader_;
  ffglex::FFGLScreenQuad quad_;
};
