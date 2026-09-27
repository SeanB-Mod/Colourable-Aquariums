// Reject ambiguous overlap rather than editing a neighbouring aquarium.
struct P56Bounds { float x,y,z,ex,ey,ez; };
static int p56_unique_water(float x,float y,float z,const P56Bounds* bounds,int count){
    if(!(x>-1e8f&&x<1e8f&&y>-1e8f&&y<1e8f&&z>-1e8f&&z<1e8f)||!bounds||count<1||count>32)return -1;
    int selected=-1;
    for(int i=0;i<count;++i){
        const P56Bounds& b=bounds[i];
        if(!(b.ex>0&&b.ex<1e8f&&b.ez>0&&b.ez<1e8f&&b.x>-1e8f&&b.x<1e8f&&b.z>-1e8f&&b.z<1e8f))continue;
        if(x<b.x-b.ex||x>b.x+b.ex||z<b.z-b.ez||z>b.z+b.ez)continue;
        if(selected>=0)return -1;
        selected=i;
    }
    return selected;
}
