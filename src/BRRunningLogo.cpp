#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#ifdef NOMETAFILE
#undef NOMETAFILE
#endif
#ifdef NOGDI
#undef NOGDI
#endif
#include <windows.h>
#include <wingdi.h>
#include <commdlg.h>
#include <objidl.h>
#include <ole2.h>
#include <gdiplus.h>

#include "BRRunningLogo.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <sstream>
#include <utility>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comdlg32.lib")

using namespace Gdiplus;
using namespace ffglex;

namespace {
enum Param : FFUInt32 {
  P_START, P_PAUSE, P_RESUME, P_RESTART,
  P_DIRECTION, P_SPEED,
  P_ANIM_IN, P_IN_AXIS, P_IN_INVERT, P_IN_DURATION,
  P_ANIM_OUT, P_OUT_AXIS, P_OUT_INVERT, P_OUT_DURATION, P_PLAY_OUT,
  P_MANAGE_LOGOS, P_PATHS,
  P_SIZE_MODE, P_SIMILAR_HEIGHT, P_LOGO_SCALE, P_ALIGN,
  P_LOGO_GAP, P_LOOP_END_GAP,
  P_BOX_WIDTH, P_BOX_HEIGHT, P_PADDING_X, P_PADDING_Y,
  P_BOX_H, P_BOX_S, P_BOX_B, P_BOX_A, P_BOX_OPACITY, P_RADIUS,
  P_BOX_STROKE, P_STROKE_WIDTH, P_STROKE_H, P_STROKE_S, P_STROKE_B, P_STROKE_A,
  P_POS_X, P_POS_Y, P_ROTATION,
  P_CREDIT,
  P_COUNT
};

static CFFGLPluginInfo PluginInfo(
  PluginFactory<BRRunningLogo>, "BRLG", "BR Running Logo",
  2, 1, 0, 101, FF_SOURCE,
  "Multi-logo ticker source by BELAJAR RESOLUME",
  "Effect by Belajar Resolume");

float brClamp01(float value){return std::max(0.0f,std::min(1.0f,value));}

std::wstring utf8ToWide(const std::string& value){
  if(value.empty())return L"";
  const int count=MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,nullptr,0);
  if(count<=0)return std::wstring(value.begin(),value.end());
  std::wstring result(static_cast<size_t>(count),L'\0');
  MultiByteToWideChar(CP_UTF8,0,value.c_str(),-1,result.data(),count);
  result.pop_back(); return result;
}

std::string wideToUtf8(const std::wstring& value){
  if(value.empty())return {};
  const int count=WideCharToMultiByte(CP_UTF8,0,value.c_str(),-1,nullptr,0,nullptr,nullptr);
  std::string result(static_cast<size_t>(count),'\0');
  WideCharToMultiByte(CP_UTF8,0,value.c_str(),-1,result.data(),count,nullptr,nullptr);
  result.pop_back(); return result;
}

const char* vertexShader=R"GLSL(#version 410 core
layout(location=0) in vec4 vPosition;
layout(location=1) in vec2 vUV;
out vec2 uv;
void main(){gl_Position=vPosition;uv=vUV;}
)GLSL";

const char* fragmentShader=R"GLSL(#version 410 core
uniform sampler2D logoTex;
uniform vec2 resolution;
uniform vec2 atlasSize;
uniform vec2 boxSize;
uniform vec2 boxCenter;
uniform vec2 paddingPx;
uniform vec4 boxColor;
uniform vec4 strokeColor;
uniform float radiusPx;
uniform float strokeWidth;
uniform float strokeEnabled;
uniform float rotationRad;
uniform float elapsed;
uniform float directionMode;
uniform float motionSpeed;
uniform float logoScale;
uniform float logoAlign;
uniform float loopEndGap;
uniform vec2 entranceOffset;
in vec2 uv;
out vec4 fragColor;

float roundedBox(vec2 p,vec2 halfSize,float radius){
  vec2 q=abs(p)-halfSize+vec2(radius);
  return length(max(q,0.0))+min(max(q.x,q.y),0.0)-radius;
}

void main(){
  vec2 pixel=uv*resolution;
  vec2 p=pixel-boxCenter-entranceOffset;
  float c=cos(-rotationRad),s=sin(-rotationRad);
  p=mat2(c,-s,s,c)*p;
  vec2 halfBox=boxSize*0.5;
  float d=roundedBox(p,halfBox,radiusPx);
  vec4 result=vec4(0.0);
  if(d<=0.0)result=boxColor;
  if(strokeEnabled>0.5){
    float band=1.0-smoothstep(strokeWidth-0.75,strokeWidth+0.75,abs(d));
    result=mix(result,strokeColor,band*strokeColor.a);
  }

  vec2 inner=max(boxSize-paddingPx*2.0,vec2(1.0));
  vec2 local=p+halfBox-paddingPx;
  if(d<=0.0 && all(greaterThanEqual(local,vec2(0.0))) && all(lessThanEqual(local,inner))){
    vec2 size=max(atlasSize*logoScale,vec2(1.0));
    vec2 q=vec2(-1.0);
    float travel=elapsed*motionSpeed;
    bool vertical=directionMode==3.0||directionMode==4.0||directionMode==6.0;
    if(!vertical){
      float y0=logoAlign<0.5?0.0:(logoAlign<1.5?(inner.y-size.y)*0.5:inner.y-size.y);
      if(directionMode==0.0){float x0=(inner.x-size.x)*0.5;q=(local-vec2(x0,y0))/size;}
      else if(directionMode==5.0){float range=max(inner.x-size.x,0.0);float x0=range*(0.5+0.5*sin(travel/max(range,1.0)*3.14159265));q=(local-vec2(x0,y0))/size;}
      else {float cycle=size.x+loopEndGap;float x=directionMode==1.0?mod(local.x+travel,cycle):mod(local.x-travel+cycle*4096.0,cycle);q=vec2(x/size.x,(local.y-y0)/size.y);}
    }else{
      float x0=(inner.x-size.x)*0.5;
      if(directionMode==6.0){float range=max(inner.y-size.y,0.0);float y0=range*(0.5+0.5*sin(travel/max(range,1.0)*3.14159265));q=(local-vec2(x0,y0))/size;}
      else {float cycle=size.y+loopEndGap;float y=directionMode==3.0?mod(local.y+travel,cycle):mod(local.y-travel+cycle*4096.0,cycle);q=vec2((local.x-x0)/size.x,y/size.y);}
    }
    if(all(greaterThanEqual(q,vec2(0.0)))&&all(lessThanEqual(q,vec2(1.0)))){
      q.y=1.0-q.y;
      vec4 logo=texture(logoTex,q);
      result=logo+result*(1.0-logo.a);
    }
  }
  fragColor=result;
}
)GLSL";
}

BRRunningLogo::BRRunningLogo():values_(P_COUNT,0.0f){
  SetMinInputs(0);SetMaxInputs(0);
  GdiplusStartupInput startup;GdiplusStartup(&gdiplusToken_,&startup,nullptr);
  values_[P_DIRECTION]=1.0f;values_[P_SPEED]=0.18f;
  values_[P_IN_DURATION]=0.25f;values_[P_OUT_DURATION]=0.25f;
  values_[P_SIMILAR_HEIGHT]=0.35f;values_[P_LOGO_SCALE]=0.35f;values_[P_ALIGN]=1.0f;
  values_[P_LOGO_GAP]=0.12f;values_[P_LOOP_END_GAP]=0.2f;
  values_[P_BOX_WIDTH]=1.0f;values_[P_BOX_HEIGHT]=0.28f;
  values_[P_PADDING_X]=0.03f;values_[P_PADDING_Y]=0.03f;
  values_[P_BOX_B]=0.08f;values_[P_BOX_A]=1.0f;values_[P_BOX_OPACITY]=0.0f;
  values_[P_RADIUS]=0.04f;values_[P_STROKE_WIDTH]=0.03f;
  values_[P_STROKE_B]=1.0f;values_[P_STROKE_A]=1.0f;
  values_[P_POS_X]=0.5f;values_[P_POS_Y]=0.5f;

  SetParamInfof(P_START,"Start",FF_TYPE_EVENT);SetParamInfof(P_PAUSE,"Pause",FF_TYPE_EVENT);
  SetParamInfof(P_RESUME,"Resume",FF_TYPE_EVENT);SetParamInfof(P_RESTART,"Restart",FF_TYPE_EVENT);
  SetOptionParamInfo(P_DIRECTION,"Direction",7,1);
  const char* directions[]={"Static","Left","Right","Up","Down","Bounce X","Bounce Y"};
  for(unsigned int i=0;i<7;++i)addOption(P_DIRECTION,i,directions[i],float(i));
  SetParamInfof(P_SPEED,"Speed",FF_TYPE_STANDARD);
  SetParamInfof(P_ANIM_IN,"Animation In",FF_TYPE_BOOLEAN);
  SetOptionParamInfo(P_IN_AXIS,"In Axis",2,0);addOption(P_IN_AXIS,0,"Scroll X",0);addOption(P_IN_AXIS,1,"Scroll Y",1);
  SetParamInfof(P_IN_INVERT,"Invert In",FF_TYPE_BOOLEAN);SetParamInfof(P_IN_DURATION,"In Duration",FF_TYPE_STANDARD);
  SetParamInfof(P_ANIM_OUT,"Animation Out",FF_TYPE_BOOLEAN);
  SetOptionParamInfo(P_OUT_AXIS,"Out Axis",2,0);addOption(P_OUT_AXIS,0,"Scroll X",0);addOption(P_OUT_AXIS,1,"Scroll Y",1);
  SetParamInfof(P_OUT_INVERT,"Invert Out",FF_TYPE_BOOLEAN);SetParamInfof(P_OUT_DURATION,"Out Duration",FF_TYPE_STANDARD);
  SetParamInfof(P_PLAY_OUT,"Play Out",FF_TYPE_EVENT);

  SetParamInfof(P_MANAGE_LOGOS,"Manage Logos",FF_TYPE_EVENT);
  SetParamInfo(P_PATHS,"Logo Paths / Preset",FF_TYPE_TEXT,serializedPaths_.c_str());
  SetOptionParamInfo(P_SIZE_MODE,"Logo Size Mode",2,0);addOption(P_SIZE_MODE,0,"Original / Individual",0);addOption(P_SIZE_MODE,1,"Similar Size",1);
  SetParamInfof(P_SIMILAR_HEIGHT,"Similar Height",FF_TYPE_STANDARD);SetParamInfof(P_LOGO_SCALE,"Global Logo Scale",FF_TYPE_STANDARD);
  SetOptionParamInfo(P_ALIGN,"Logo Align",3,1);addOption(P_ALIGN,0,"Top",0);addOption(P_ALIGN,1,"Center",1);addOption(P_ALIGN,2,"Bottom",2);
  SetParamInfof(P_LOGO_GAP,"Logo Gap",FF_TYPE_STANDARD);SetParamInfof(P_LOOP_END_GAP,"Loop End Gap",FF_TYPE_STANDARD);

  SetParamInfof(P_BOX_WIDTH,"Box Width",FF_TYPE_STANDARD);SetParamInfof(P_BOX_HEIGHT,"Box Height",FF_TYPE_STANDARD);
  SetParamInfof(P_PADDING_X,"Padding Left Right",FF_TYPE_STANDARD);SetParamInfof(P_PADDING_Y,"Padding Top Bottom",FF_TYPE_STANDARD);
  SetParamInfof(P_BOX_H,"Box Color",FF_TYPE_HUE);SetParamInfof(P_BOX_S,"Box Color Sat",FF_TYPE_SATURATION);
  SetParamInfof(P_BOX_B,"Box Color Bri",FF_TYPE_BRIGHTNESS);SetParamInfof(P_BOX_A,"Box Color Alpha",FF_TYPE_ALPHA);
  SetParamInfof(P_BOX_OPACITY,"Box Opacity",FF_TYPE_STANDARD);SetParamInfof(P_RADIUS,"Corner Radius",FF_TYPE_STANDARD);
  SetParamInfof(P_BOX_STROKE,"Box Stroke",FF_TYPE_BOOLEAN);SetParamInfof(P_STROKE_WIDTH,"Box Stroke Width",FF_TYPE_STANDARD);
  SetParamInfof(P_STROKE_H,"Stroke Color",FF_TYPE_HUE);SetParamInfof(P_STROKE_S,"Stroke Color Sat",FF_TYPE_SATURATION);
  SetParamInfof(P_STROKE_B,"Stroke Color Bri",FF_TYPE_BRIGHTNESS);SetParamInfof(P_STROKE_A,"Stroke Color Alpha",FF_TYPE_ALPHA);
  SetParamInfof(P_POS_X,"Position X",FF_TYPE_XPOS);SetParamInfof(P_POS_Y,"Position Y",FF_TYPE_YPOS);
  SetOptionParamInfo(P_ROTATION,"Rotation",4,0);addOption(P_ROTATION,0,"0 degrees",0);addOption(P_ROTATION,1,"90 degrees",1);addOption(P_ROTATION,2,"180 degrees",2);addOption(P_ROTATION,3,"270 degrees",3);
  SetParamInfo(P_CREDIT,"Effect by Belajar Resolume",FF_TYPE_TEXT,credit_.c_str());

  for(unsigned int i=P_START;i<=P_PLAY_OUT;++i)SetParamGroup(i,"Animation");
  for(unsigned int i=P_MANAGE_LOGOS;i<=P_LOOP_END_GAP;++i)SetParamGroup(i,"Logo Setting");
  for(unsigned int i=P_BOX_WIDTH;i<=P_ROTATION;++i)SetParamGroup(i,"Box Setting");
  SetParamGroup(P_CREDIT,"Credit");
  lastFrame_=std::chrono::steady_clock::now();
}

BRRunningLogo::~BRRunningLogo(){
  managerStopRequested_.store(true);
  if(HWND window=managerWindow_.load())PostMessageW(window,WM_CLOSE,0,0);
  if(managerThread_.joinable())managerThread_.join();
  if(gdiplusToken_)GdiplusShutdown(gdiplusToken_);
}

void BRRunningLogo::addOption(unsigned int p,unsigned int e,const char* n,float v){SetParamElementInfo(p,e,n,v);}

std::string BRRunningLogo::serializePaths(const std::vector<std::string>& paths)const{
  std::ostringstream stream;for(size_t i=0;i<paths.size();++i){if(i)stream<<'|';stream<<paths[i];}return stream.str();
}

std::vector<std::string> BRRunningLogo::parsePaths(const std::string& value)const{
  std::vector<std::string> result;std::stringstream stream(value);std::string item;
  while(std::getline(stream,item,'|'))if(!item.empty())result.push_back(item);return result;
}

void BRRunningLogo::setLogoPaths(const std::vector<std::string>& paths){
  {std::lock_guard<std::mutex> lock(logoMutex_);logoPaths_=paths;serializedPaths_=serializePaths(paths);}
  atlasDirty_.store(true);pathsEventPending_.store(true);
}

std::vector<std::string> BRRunningLogo::getLogoPaths(){std::lock_guard<std::mutex> lock(logoMutex_);return logoPaths_;}

void BRRunningLogo::openLogoManager(){
  if(managerRunning_.load()){if(HWND w=managerWindow_.load())PostMessageW(w,WM_APP+1,0,0);return;}
  if(managerThread_.joinable())managerThread_.join();
  managerStopRequested_.store(false);managerRunning_.store(true);
  managerThread_=std::thread(&BRRunningLogo::runLogoManager,this);
}

void BRRunningLogo::runLogoManager(){
  static const wchar_t* className=L"BRRunningLogoManagerWindow";
  HINSTANCE instance=GetModuleHandleW(nullptr);
  WNDCLASSEXW wc{};wc.cbSize=sizeof(wc);wc.lpfnWndProc=ManagerWndProc;wc.hInstance=instance;
  wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);wc.lpszClassName=className;
  if(RegisterClassExW(&wc)==0&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS){managerRunning_.store(false);return;}
  HWND window=CreateWindowExW(WS_EX_TOOLWINDOW,className,L"BR Running Logo — Logo Manager",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    CW_USEDEFAULT,CW_USEDEFAULT,820,540,nullptr,nullptr,instance,this);
  if(!window){managerRunning_.store(false);return;}managerWindow_.store(window);
  if(managerStopRequested_.load())PostMessageW(window,WM_CLOSE,0,0);
  MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
  managerWindow_.store(nullptr);managerRunning_.store(false);
}

LRESULT CALLBACK BRRunningLogo::ManagerWndProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam){
  auto* self=reinterpret_cast<BRRunningLogo*>(GetWindowLongPtrW(window,GWLP_USERDATA));
  if(message==WM_NCCREATE){auto* c=reinterpret_cast<CREATESTRUCTW*>(lParam);self=reinterpret_cast<BRRunningLogo*>(c->lpCreateParams);SetWindowLongPtrW(window,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(self));}
  if(!self)return DefWindowProcW(window,message,wParam,lParam);
  constexpr int ID_LIST=2001,ID_ADD=2002,ID_REMOVE=2003,ID_UP=2004,ID_DOWN=2005,ID_CLEAR=2006,ID_CANCEL=2007,ID_APPLY=2008;
  auto moveItem=[&](int delta){
    int index=int(SendMessageW(self->managerList_,LB_GETCURSEL,0,0));if(index==LB_ERR)return;
    int target=index+delta;int count=int(SendMessageW(self->managerList_,LB_GETCOUNT,0,0));if(target<0||target>=count)return;
    int length=int(SendMessageW(self->managerList_,LB_GETTEXTLEN,index,0));std::wstring text(static_cast<size_t>(length)+1,L'\0');
    SendMessageW(self->managerList_,LB_GETTEXT,index,reinterpret_cast<LPARAM>(text.data()));text.resize(static_cast<size_t>(length));
    SendMessageW(self->managerList_,LB_DELETESTRING,index,0);SendMessageW(self->managerList_,LB_INSERTSTRING,target,reinterpret_cast<LPARAM>(text.c_str()));SendMessageW(self->managerList_,LB_SETCURSEL,target,0);
  };
  switch(message){
    case WM_CREATE:{
      CreateWindowExW(0,L"STATIC",L"Add multiple PNG/JPG/BMP logos. The list order is the running order.",WS_CHILD|WS_VISIBLE,16,14,760,22,window,nullptr,nullptr,nullptr);
      self->managerList_=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,
        16,42,610,420,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LIST)),nullptr,nullptr);
      const struct{int id;const wchar_t* label;int y;} buttons[]={{ID_ADD,L"Add Logos",42},{ID_REMOVE,L"Remove",82},{ID_UP,L"Move Up",122},{ID_DOWN,L"Move Down",162},{ID_CLEAR,L"Clear All",202}};
      for(const auto& b:buttons)CreateWindowExW(0,L"BUTTON",b.label,WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,642,b.y,140,32,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(b.id)),nullptr,nullptr);
      CreateWindowExW(0,L"BUTTON",L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,566,470,104,34,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CANCEL)),nullptr,nullptr);
      CreateWindowExW(0,L"BUTTON",L"Apply Logos",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,678,470,104,34,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_APPLY)),nullptr,nullptr);
      for(const auto& path:self->getLogoPaths()){std::wstring wide=utf8ToWide(path);SendMessageW(self->managerList_,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(wide.c_str()));}
      return 0;
    }
    case WM_SIZE:{
      int width=LOWORD(lParam),height=HIWORD(lParam);if(self->managerList_)MoveWindow(self->managerList_,16,42,std::max(260,width-190),std::max(150,height-120),TRUE);
      const int x=std::max(292,width-174);const int ids[]={ID_ADD,ID_REMOVE,ID_UP,ID_DOWN,ID_CLEAR};for(int i=0;i<5;++i)if(HWND b=GetDlgItem(window,ids[i]))MoveWindow(b,x,42+i*40,150,32,TRUE);
      if(HWND b=GetDlgItem(window,ID_CANCEL))MoveWindow(b,std::max(16,width-238),std::max(230,height-50),104,34,TRUE);
      if(HWND b=GetDlgItem(window,ID_APPLY))MoveWindow(b,std::max(128,width-126),std::max(230,height-50),110,34,TRUE);return 0;
    }
    case WM_COMMAND:{
      const int id=LOWORD(wParam);
      if(id==ID_ADD){
        std::vector<wchar_t> buffer(65536,L'\0');OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;
        dialog.lpstrFilter=L"Image Files\0*.png;*.jpg;*.jpeg;*.bmp\0PNG\0*.png\0JPEG\0*.jpg;*.jpeg\0Bitmap\0*.bmp\0\0";
        dialog.lpstrFile=buffer.data();dialog.nMaxFile=DWORD(buffer.size());dialog.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_PATHMUSTEXIST;
        if(GetOpenFileNameW(&dialog)){
          const wchar_t* first=buffer.data();const wchar_t* second=first+wcslen(first)+1;
          if(*second==L'\0')SendMessageW(self->managerList_,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(first));
          else{std::wstring folder=first;for(const wchar_t* name=second;*name;name+=wcslen(name)+1){std::wstring full=folder+L"\\"+name;SendMessageW(self->managerList_,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(full.c_str()));}}
        }return 0;
      }
      if(id==ID_REMOVE){int index=int(SendMessageW(self->managerList_,LB_GETCURSEL,0,0));if(index!=LB_ERR)SendMessageW(self->managerList_,LB_DELETESTRING,index,0);return 0;}
      if(id==ID_UP){moveItem(-1);return 0;}if(id==ID_DOWN){moveItem(1);return 0;}
      if(id==ID_CLEAR){SendMessageW(self->managerList_,LB_RESETCONTENT,0,0);return 0;}
      if(id==ID_CANCEL){DestroyWindow(window);return 0;}
      if(id==ID_APPLY){
        std::vector<std::string> paths;int count=int(SendMessageW(self->managerList_,LB_GETCOUNT,0,0));
        for(int i=0;i<count;++i){int length=int(SendMessageW(self->managerList_,LB_GETTEXTLEN,i,0));std::wstring value(static_cast<size_t>(length)+1,L'\0');SendMessageW(self->managerList_,LB_GETTEXT,i,reinterpret_cast<LPARAM>(value.data()));value.resize(static_cast<size_t>(length));paths.push_back(wideToUtf8(value));}
        self->setLogoPaths(paths);DestroyWindow(window);return 0;
      }break;
    }
    case WM_APP+1:ShowWindow(window,SW_RESTORE);SetForegroundWindow(window);SetFocus(self->managerList_);return 0;
    case WM_CLOSE:DestroyWindow(window);return 0;
    case WM_NCDESTROY:self->managerList_=nullptr;self->managerWindow_.store(nullptr);SetWindowLongPtrW(window,GWLP_USERDATA,0);PostQuitMessage(0);break;
  }
  return DefWindowProcW(window,message,wParam,lParam);
}

FFResult BRRunningLogo::InitGL(const FFGLViewportStruct* viewport){
  viewportWidth_=viewport->width;viewportHeight_=viewport->height;
  if(!shader_.Compile(vertexShader,fragmentShader)||!quad_.Initialise())return FF_FAIL;
  atlasDirty_.store(false);rebuildAtlas();return CFFGLPlugin::InitGL(viewport);
}

void BRRunningLogo::releaseAtlas(){if(atlasTexture_){glDeleteTextures(1,&atlasTexture_);atlasTexture_=0;}}

void BRRunningLogo::rebuildAtlas(){
  releaseAtlas();
  struct Logo{std::unique_ptr<Bitmap> bitmap;unsigned int width=1,height=1,drawW=1,drawH=1;};
  std::vector<Logo> logos;unsigned int maxOriginalHeight=1;
  for(const auto& path:getLogoPaths()){
    auto bitmap=std::make_unique<Bitmap>(utf8ToWide(path).c_str());
    if(bitmap->GetLastStatus()!=Ok||bitmap->GetWidth()==0||bitmap->GetHeight()==0)continue;
    Logo logo;logo.width=bitmap->GetWidth();logo.height=bitmap->GetHeight();maxOriginalHeight=std::max(maxOriginalHeight,logo.height);logo.bitmap=std::move(bitmap);logos.push_back(std::move(logo));
  }
  const bool similar=values_[P_SIZE_MODE]>0.5f;const float similarHeight=64.0f+values_[P_SIMILAR_HEIGHT]*448.0f;
  const float originalScale=std::min(1.0f,512.0f/float(maxOriginalHeight));
  for(auto& logo:logos){float scale=similar?similarHeight/float(logo.height):originalScale;logo.drawW=std::max(1u,unsigned(std::round(logo.width*scale)));logo.drawH=std::max(1u,unsigned(std::round(logo.height*scale)));}
  unsigned int gap=unsigned(8.0f+values_[P_LOGO_GAP]*392.0f);const int direction=int(values_[P_DIRECTION]+0.5f);const bool vertical=direction==3||direction==4||direction==6;
  auto calculate=[&](){unsigned int w=1,h=1;if(!logos.empty()){if(vertical){w=0;h=0;for(size_t i=0;i<logos.size();++i){w=std::max(w,logos[i].drawW);h+=logos[i].drawH+(i+1<logos.size()?gap:0);}}else{w=0;h=0;for(size_t i=0;i<logos.size();++i){w+=logos[i].drawW+(i+1<logos.size()?gap:0);h=std::max(h,logos[i].drawH);}}}return std::pair<unsigned int,unsigned int>(std::max(1u,w),std::max(1u,h));};
  auto dimensions=calculate();float fit=std::min(1.0f,8192.0f/float(std::max(dimensions.first,dimensions.second)));
  if(fit<1.0f){gap=std::max(1u,unsigned(gap*fit));for(auto& logo:logos){logo.drawW=std::max(1u,unsigned(logo.drawW*fit));logo.drawH=std::max(1u,unsigned(logo.drawH*fit));}dimensions=calculate();}
  atlasWidth_=dimensions.first;atlasHeight_=dimensions.second;
  Bitmap atlas(atlasWidth_,atlasHeight_,PixelFormat32bppARGB);Graphics graphics(&atlas);graphics.SetCompositingMode(CompositingModeSourceCopy);graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);graphics.Clear(Color(0,0,0,0));
  unsigned int cursor=0;for(auto& logo:logos){int x=0,y=0;if(vertical){x=int(atlasWidth_-logo.drawW)/2;y=int(cursor);cursor+=logo.drawH+gap;}else{int align=int(values_[P_ALIGN]+0.5f);x=int(cursor);y=align==0?0:(align==1?int(atlasHeight_-logo.drawH)/2:int(atlasHeight_-logo.drawH));cursor+=logo.drawW+gap;}graphics.DrawImage(logo.bitmap.get(),Rect(x,y,int(logo.drawW),int(logo.drawH)),0,0,int(logo.width),int(logo.height),UnitPixel);}
  Rect rect(0,0,atlasWidth_,atlasHeight_);BitmapData data{};atlas.LockBits(&rect,ImageLockModeRead,PixelFormat32bppARGB,&data);
  GLint oldTexture=0,oldAlignment=4,oldRowLength=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&oldTexture);glGetIntegerv(GL_UNPACK_ALIGNMENT,&oldAlignment);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&oldRowLength);
  glGenTextures(1,&atlasTexture_);glBindTexture(GL_TEXTURE_2D,atlasTexture_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,atlasWidth_,atlasHeight_,0,GL_BGRA,GL_UNSIGNED_BYTE,data.Scan0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,oldAlignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,oldRowLength);glBindTexture(GL_TEXTURE_2D,GLuint(oldTexture));atlas.UnlockBits(&data);
}

FFResult BRRunningLogo::ProcessOpenGL(ProcessOpenGLStruct*){
  if(pathsEventPending_.exchange(false))RaiseParamEvent(P_PATHS,FF_EVENT_FLAG_VALUE);
  if(!visible_)return FF_SUCCESS;if(atlasDirty_.exchange(false))rebuildAtlas();
  auto now=std::chrono::steady_clock::now();double delta=std::chrono::duration<double>(now-lastFrame_).count();lastFrame_=now;if(!paused_)elapsed_+=std::min(delta,0.1);
  float offsetX=0,offsetY=0;
  if(values_[P_ANIM_IN]>0.5f){float duration=.05f+values_[P_IN_DURATION]*4.95f;float progress=brClamp01(float(elapsed_/duration));progress=progress*progress*(3-2*progress);float distance=values_[P_IN_AXIS]<.5f?float(viewportWidth_):float(viewportHeight_);float offset=(1-progress)*distance*(values_[P_IN_INVERT]>.5f?1:-1);if(values_[P_IN_AXIS]<.5f)offsetX=offset;else offsetY=offset;}
  if(playingOut_&&values_[P_ANIM_OUT]>.5f){float duration=.05f+values_[P_OUT_DURATION]*4.95f;float progress=brClamp01(float((elapsed_-outStartedAt_)/duration));progress=progress*progress*(3-2*progress);float distance=values_[P_OUT_AXIS]<.5f?float(viewportWidth_):float(viewportHeight_);float offset=progress*distance*(values_[P_OUT_INVERT]>.5f?-1:1);if(values_[P_OUT_AXIS]<.5f)offsetX=offset;else offsetY=offset;if(progress>=1.0f)visible_=false;}
  float boxRgb[3],strokeRgb[3];HSVtoRGB(values_[P_BOX_H],values_[P_BOX_S],values_[P_BOX_B],boxRgb[0],boxRgb[1],boxRgb[2]);HSVtoRGB(values_[P_STROKE_H],values_[P_STROKE_S],values_[P_STROKE_B],strokeRgb[0],strokeRgb[1],strokeRgb[2]);
  float boxW=std::max(8.0f,values_[P_BOX_WIDTH]*viewportWidth_),boxH=std::max(8.0f,values_[P_BOX_HEIGHT]*viewportHeight_);
  ScopedShaderBinding binding(shader_.GetGLID());glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,atlasTexture_);auto u=[&](const char* name){return shader_.FindUniform(name);};
  glUniform1i(u("logoTex"),0);glUniform2f(u("resolution"),float(viewportWidth_),float(viewportHeight_));glUniform2f(u("atlasSize"),float(atlasWidth_),float(atlasHeight_));glUniform2f(u("boxSize"),boxW,boxH);
  glUniform2f(u("boxCenter"),values_[P_POS_X]*viewportWidth_,(1-values_[P_POS_Y])*viewportHeight_);glUniform2f(u("paddingPx"),values_[P_PADDING_X]*viewportWidth_*.2f,values_[P_PADDING_Y]*viewportHeight_*.2f);
  glUniform4f(u("boxColor"),boxRgb[0],boxRgb[1],boxRgb[2],values_[P_BOX_A]*values_[P_BOX_OPACITY]);glUniform4f(u("strokeColor"),strokeRgb[0],strokeRgb[1],strokeRgb[2],values_[P_STROKE_A]);
  glUniform1f(u("radiusPx"),values_[P_RADIUS]*std::min(boxW,boxH)*.5f);glUniform1f(u("strokeWidth"),1+values_[P_STROKE_WIDTH]*30);glUniform1f(u("strokeEnabled"),values_[P_BOX_STROKE]);
  glUniform1f(u("rotationRad"),values_[P_ROTATION]*1.57079632679f);glUniform1f(u("elapsed"),float(elapsed_));glUniform1f(u("directionMode"),values_[P_DIRECTION]);glUniform1f(u("motionSpeed"),20+values_[P_SPEED]*700);
  glUniform1f(u("logoScale"),.2f+values_[P_LOGO_SCALE]*1.8f);glUniform1f(u("logoAlign"),values_[P_ALIGN]);glUniform1f(u("loopEndGap"),values_[P_LOOP_END_GAP]*600);glUniform2f(u("entranceOffset"),offsetX,offsetY);
  glEnable(GL_BLEND);glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);quad_.Draw();glDisable(GL_BLEND);glBindTexture(GL_TEXTURE_2D,0);return FF_SUCCESS;
}

FFResult BRRunningLogo::DeInitGL(){releaseAtlas();shader_.FreeGLResources();quad_.Release();return FF_SUCCESS;}

FFResult BRRunningLogo::SetFloatParameter(unsigned int index,float value){
  if(index>=P_COUNT)return FF_FAIL;values_[index]=value;
  if(index==P_MANAGE_LOGOS&&value!=0)openLogoManager();
  if(index==P_START&&value!=0){visible_=true;paused_=false;playingOut_=false;elapsed_=0;lastFrame_=std::chrono::steady_clock::now();}
  if(index==P_PAUSE&&value!=0)paused_=true;
  if(index==P_RESUME&&value!=0){visible_=true;paused_=false;lastFrame_=std::chrono::steady_clock::now();}
  if(index==P_RESTART&&value!=0){visible_=true;playingOut_=false;elapsed_=0;lastFrame_=std::chrono::steady_clock::now();}
  if(index==P_PLAY_OUT&&value!=0){if(values_[P_ANIM_OUT]>.5f){playingOut_=true;paused_=false;outStartedAt_=elapsed_;}else visible_=false;}
  if(index==P_SIZE_MODE||index==P_SIMILAR_HEIGHT||index==P_ALIGN||index==P_LOGO_GAP||index==P_DIRECTION)atlasDirty_.store(true);
  return FF_SUCCESS;
}

float BRRunningLogo::GetFloatParameter(unsigned int index){return index<P_COUNT?values_[index]:0;}

FFResult BRRunningLogo::SetTextParameter(unsigned int index,const char* value){
  if(index==P_PATHS){auto paths=parsePaths(value?value:"");{std::lock_guard<std::mutex> lock(logoMutex_);logoPaths_=paths;serializedPaths_=serializePaths(paths);}atlasDirty_.store(true);return FF_SUCCESS;}if(index==P_CREDIT)return FF_SUCCESS;return FF_FAIL;
}

char* BRRunningLogo::GetTextParameter(unsigned int index){
  static thread_local char buffer[32768];std::string value;if(index==P_CREDIT)value=credit_;else if(index==P_PATHS){std::lock_guard<std::mutex> lock(logoMutex_);value=serializedPaths_;}else return buffer;
  size_t length=std::min(value.size(),sizeof(buffer)-1);std::memcpy(buffer,value.data(),length);buffer[length]=0;return buffer;
}
