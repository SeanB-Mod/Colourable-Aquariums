#include <cassert>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include "LightingMath.h"
int main(){
 P55RGB retail{5.91121f,10.35887f,18.89791f};
 for(int r=0;r<=10;++r)for(int g=0;g<=10;++g)for(int b=0;b<=10;++b){
  P55RGB picker{r/10.f,g/10.f,b/10.f};auto c=p67_soft_glow(retail,picker);
  assert(p55_valid_rgb(c));assert(p55_luminance(c)<=p55_luminance(retail)*1.00001f);
  assert(c.r<=18.89791f*3.334f&&c.g<=18.89791f*3.334f&&c.b<=18.89791f*3.334f);
  float level=std::fmin(1.f,.5f+2.f*std::fmax(picker.r,std::fmax(picker.g,picker.b)));
  assert(std::fabs(p55_luminance(c)-p55_luminance(retail)*level)<.0001f);
  auto half=p67_soft_glow(retail,{picker.r*.5f,picker.g*.5f,picker.b*.5f});
  float v=std::fmax(picker.r,std::fmax(picker.g,picker.b));float ratio=std::fmin(1.f,.5f+v)/std::fmin(1.f,.5f+2.f*v);
  assert(std::fabs(half.r-c.r*ratio)<.0001f&&std::fabs(half.g-c.g*ratio)<.0001f&&std::fabs(half.b-c.b*ratio)<.0001f);
 }
 auto black=p67_soft_glow(retail,{0,0,0});auto white=p67_soft_glow(retail,{1,1,1});assert(std::fabs(black.r-white.r*.5f)<.0001f);
 for(float v: {0.125f,0.2499f,0.25f,0.2501f,0.5f,1.f}){auto c=p67_soft_glow(retail,{v,v,v});float expected=std::fmin(1.f,.5f+2.f*v);assert(std::fabs(c.r-white.r*expected)<.0001f);}
 auto invalid=p67_soft_glow(retail,{-1,0,0});assert(invalid.r==retail.r);
 std::puts("Soft glow passed: 1331 colours, retail luminance equality across hues and finite peaks, 50% floor, quarter-brightness plateau, black and invalid input.");
}
