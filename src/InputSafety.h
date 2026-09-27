struct P70FocusGate {
    bool previous=false;
    bool accept(bool focused){bool accepted=previous&&focused;previous=focused;return accepted;}
};
static bool p70_mouse_edge(bool down,bool previous){return down&&!previous;}
