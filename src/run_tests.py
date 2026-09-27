"""Run isolated fixtures; never launches the game or installs a DLL."""
import argparse,ctypes,pathlib,subprocess,sys,json
p=argparse.ArgumentParser();p.add_argument('--compiler',required=True);p.add_argument('--output',required=True);p.add_argument('--game-assembly');a=p.parse_args()
if sys.platform=='win32':ctypes.windll.kernel32.SetErrorMode(0x8003)
root=pathlib.Path(__file__).resolve().parent;out=pathlib.Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
compiler=str(pathlib.Path(a.compiler).resolve());results=[]
for name in ['test_swatch_update','test_palette_edit','test_reticle','test_state_cache','test_material_lifecycle','test_input_focus','test_snapshot_io','test_click','test_guard','test_handles_and_poll','test_glow_queue','test_swatches','test_discovery','test_donor','test_factory','test_reset_material']:
 subprocess.run([sys.executable,str(root/(name+'.py')),compiler,str(out)],check=True,timeout=90);results.append(name)
for name in ['test_sampling_bridge','test_palette_bridge','test_colour_bridge','test_bounded_log','test_runtime_work','test_bootstrap','test_persistence','test_soft_glow','test_ui','test_window_host','test_exception']:
 exe=out/(name+'.exe');subprocess.run([compiler,'-O2','-static',str(root/(name+'.cpp')),'-o',str(exe)],check=True,timeout=90)
 subprocess.run([str(exe)],check=True,timeout=30);results.append(name)
 if name=='test_bootstrap':subprocess.run([str(exe),'timeout'],check=True,timeout=30);results.append('test_bootstrap_timeout')
if a.game_assembly:
 subprocess.run([sys.executable,str(root/'verify_game_hooks.py'),a.game_assembly],check=True,timeout=60);results.append('verify_game_hooks')
(out/'results.json').write_text(json.dumps({'passed':results},indent=2));print(f'{len(results)} checks passed. In-game testing still required.')
