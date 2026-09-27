"""Read-only verification of exact hook bytes against the installed game."""
import pathlib,re,struct,sys
image=pathlib.Path(sys.argv[1]).read_bytes()
pe=struct.unpack_from('<I',image,0x3c)[0]
assert image[pe:pe+4]==b'PE\0\0'
sections=struct.unpack_from('<H',image,pe+6)[0]
optional=struct.unpack_from('<H',image,pe+20)[0]
table=pe+24+optional
def read(rva,size):
    for n in range(sections):
        off=table+40*n
        virtual_size,address,raw_size,raw=struct.unpack_from('<IIII',image,off+8)
        if address<=rva and rva+size<=address+raw_size:
            return image[raw+rva-address:raw+rva-address+size]
    raise ValueError(hex(rva))
source=pathlib.Path(__file__).with_name('AquariumUI.h').read_text()
targets={'update':0x2aa69b0,'refresh':0x29af470,'close':0x29292c0,'input':0x29297b0,'press':0x20dfdb0,'force':0x20dff50}
for name,rva in targets.items():
    body=re.search(r'const u8 '+name+r'\[\]=\{([^}]+)\}',source)[1]
    expected=bytes(int(v.strip(),0) for v in body.split(','))
    assert read(rva,len(expected))==expected, f'{name}: prologue mismatch'
    print(f'{name}: exact {len(expected)} bytes verified at RVA {rva:#x}')
for file, marker, rva in [('AquariumColour_source.cpp','static bool p51_install_post_update_hook(){',0x31fb310),('LightingConsumer.h','static bool p55_install_consumer_hook(){',0x3516980)]:
    text=pathlib.Path(__file__).with_name(file).read_text().split(marker,1)[1]
    body=re.search(r'const u8 expected\[\d+\]=\{([^}]+)\}',text)[1]
    expected=bytes(int(v.strip(),0) for v in body.split(','))
    assert read(rva,len(expected))==expected, f'{file}: prologue mismatch'
    print(f'{file}: exact {len(expected)} bytes verified at RVA {rva:#x}')
print('All eight native hook guards match the installed game. No game files modified.')
