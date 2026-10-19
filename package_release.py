"""Package only explicitly reviewed files. Run from any directory with Python 3."""
import argparse,hashlib,io,json,pathlib,re,zipfile
p=argparse.ArgumentParser()
p.add_argument('--binary',type=pathlib.Path,required=True)
p.add_argument('--output',type=pathlib.Path)
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parent
manifest=json.loads((root/'PACKAGE_MANIFEST.json').read_text())
project=manifest['project'];version=(root/'VERSION').read_text().strip()
assert re.fullmatch(r'[0-9]+(?:\.[0-9]+){1,3}',version)
out=a.output or root/'release-assets';out.mkdir(parents=True,exist_ok=True)
stem=project+'-v'+version
runtimeName=stem+'-Windows-x64.zip';sourceName=stem+'-GitHub-Pack.zip';workshopName=stem+'-Workshop-Upload.zip'
def digest(data):return hashlib.sha256(data).hexdigest()
def safe(name):
    path=pathlib.PurePosixPath(name)
    assert not path.is_absolute() and all(x not in ('','..','.') for x in name.split('/'))
    assert not any(c in name for c in ':\\%')
    assert not re.search(r'(?i)(?:^|/)(?:Logs|Saves|ProgramData|__pycache__)(?:/|$)',name)
    assert not name.lower().endswith(('.db','.log','.pdb','.exe'))
    assert 'leveleditor' not in name.lower()
def zip_bytes(files):
    target=io.BytesIO()
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(files.items()):
            safe(name)
            info=zipfile.ZipInfo(name,(2026,9,27,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            info.external_attr=0o100644<<16;z.writestr(info,data)
    return target.getvalue()
runtime={name:(a.binary if origin=='$BINARY' else root/origin).read_bytes() for name,origin in manifest['runtime'].items()}
assert a.binary.read_bytes()[:2]==b'MZ'
assert sum(n.lower().endswith('.dll') for n in runtime)==1
# The game root contains only winhttp.dll; all other installed files are under Mods.
assert all(n=='winhttp.dll' or n.startswith('Mods/') for n in runtime)
if project=='TPM-Mod-Loader':
    ini=runtime['Mods/loader.ini'].decode()
    assert 'LevelEditor' not in ini
    assert all(n in ini for n in ('TPM-Mod-Engine.dll','TPM-ColourPicker.dll','TPM-AquaColour.dll'))
runtimeData=zip_bytes(runtime)
checksum=(digest(runtimeData)+'  '+runtimeName+'\n').encode()
source={name:(root/name).read_bytes() for name in manifest['source']}
assert len(source)==len(manifest['source'])
for name,data in source.items():
    safe(name)
    assert not name.lower().endswith(('.dll','.zip'))
    if name.endswith(('.cpp','.h','.py','.ps1','.md','.ini','.json')):
        assert bytes([83,101,97,110,45]) not in data and bytes([79,110,101,68,114,105,118,101]) not in data,(name,'personal path')
workshop={'DLL Mod/TPM-AquaColour.dll':a.binary.read_bytes()}
workshopData=zip_bytes(workshop)
workshopChecksum=(digest(workshopData)+'  '+workshopName+'\n').encode()
source['release-assets/'+workshopName]=workshopData
source['release-assets/'+workshopName+'.sha256']=workshopChecksum
source['release-assets/'+runtimeName]=runtimeData
source['release-assets/'+runtimeName+'.sha256']=checksum
sourceData=zip_bytes(source)
def audit(data,expected):
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        assert z.testzip() is None
        assert len(z.namelist())==len(set(z.namelist()))
        assert set(z.namelist())==set(expected)
        for n in z.namelist():
            safe(n);assert z.read(n)==expected[n]
            if n.endswith('.zip'):audit(z.read(n),workshop if n.endswith(workshopName) else runtime)
audit(runtimeData,runtime);audit(workshopData,workshop);audit(sourceData,source)
(out/workshopName).write_bytes(workshopData)
(out/(workshopName+'.sha256')).write_bytes(workshopChecksum)
(out/runtimeName).write_bytes(runtimeData)
(out/(runtimeName+'.sha256')).write_bytes(checksum)
(out/sourceName).write_bytes(sourceData)
(out/(sourceName+'.sha256')).write_text(digest(sourceData)+'  '+sourceName+'\n')
report={'project':project,'version':version,'audit':'passed','runtimeMembers':{n:digest(d) for n,d in runtime.items()},'sourceMembers':sorted(source),'workshopMembers':{n:digest(d) for n,d in workshop.items()},'archives':{runtimeName:digest(runtimeData),workshopName:digest(workshopData),sourceName:digest(sourceData)}}
(out/(stem+'-AUDIT.json')).write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'project':project,'runtimeFiles':len(runtime),'sourceFiles':len(source),'audit':'passed'}))
