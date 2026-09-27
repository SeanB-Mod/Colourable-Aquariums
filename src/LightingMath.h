// Retail/output are linear RGB; picker RGB supplies hue and value. No game dependencies.
struct P55RGB { float r,g,b; };
static float p55_luminance(P55RGB c){return 0.2126f*c.r+0.7152f*c.g+0.0722f*c.b;}
static bool p55_valid_rgb(P55RGB c){return c.r>=0&&c.g>=0&&c.b>=0&&c.r<100000&&c.g<100000&&c.b<100000;}
static P55RGB p55_tint_preserving_brightness(P55RGB retail,P55RGB direction){
    if(!p55_valid_rgb(retail)||!p55_valid_rgb(direction))return retail;
    float baseline=p55_luminance(retail),weight=p55_luminance(direction);
    if(baseline<=0.000001f||weight<=0.000001f)return retail;
    float scale=baseline/weight;
    P55RGB result{direction.r*scale,direction.g*scale,direction.b*scale};
    return p55_valid_rgb(result)?result:retail;
}
// Match retail luminance at quarter value and above; retain 50% at black.
static P55RGB p67_soft_glow(P55RGB retail,P55RGB picker){
    if(!p55_valid_rgb(retail)||!p55_valid_rgb(picker))return retail;
    float value=picker.r>picker.g?picker.r:picker.g;if(picker.b>value)value=picker.b;
    if(value>1)value=1;
    // 50% at black, reaching retail luminance at quarter value.
    float level=0.50f+2.0f*value;if(level>1.0f)level=1.0f;
    P55RGB direction{1,1,1};
    if(value>0.000001f)direction={0.30f+0.70f*picker.r/value,0.30f+0.70f*picker.g/value,0.30f+0.70f*picker.b/value};
    P55RGB c=p55_tint_preserving_brightness(retail,direction);
    // No second attenuation or per-channel peak cap: those made "100%" darker
    // than retail and reduced different hues by different amounts.
    return {c.r*level,c.g*level,c.b*level};
}
