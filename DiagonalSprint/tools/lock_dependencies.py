"""Maintainer-only: resolve pinned refs and record archive hashes. Normal builds use CMake."""
import concurrent.futures, hashlib, json, pathlib, subprocess, urllib.request
ROOT = pathlib.Path(__file__).resolve().parents[1]
CACHE = ROOT.parent / 'Libraries' / '.archives'
CACHE.mkdir(parents=True, exist_ok=True)
SPECS = [
 ('commonlib_ng','alandtse/CommonLibSSE-NG','a6d3ea4642f71fb92ac3e0d7fef5808217f8aae9','CMakeLists.txt'),
 ('commonlib_po3','powerof3/CommonLibSSE','431ded2','CMakeLists.txt'),
 ('directxtk','microsoft/DirectXTK','may2026','CMakeLists.txt'),
 ('spdlog','gabime/spdlog','v1.17.0','CMakeLists.txt'),
 ('xbyak','herumi/xbyak','v7.38','xbyak/xbyak.h'),
 ('imgui','ocornut/imgui','v1.92.9b','imgui.h'),
 ('json','nlohmann/json','v3.12.0','single_include/nlohmann/json.hpp'),
 ('simpleini','brofield/simpleini','v4.26','SimpleIni.h'),
 ('toml11','ToruNiina/toml11','v4.4.0','single_include/toml.hpp'),
 ('binary_io','Ryan-rsm-McKenzie/binary_io','2.0.6','CMakeLists.txt'),
 ('rapidcsv','d99kris/rapidcsv','v8.90','src/rapidcsv.h'),
 ('minhook','TsudaKageyu/minhook','v1.3.4','src/hde/hde64.c'),
 ('rapidjson','Tencent/rapidjson','v1.1.0','include/rapidjson/document.h'),
 ('skse_menu','QTR-Modding/SKSE-Menu-Framework-3-API','1dcb70179076aae4ab626f43c5baab2735ca5877','SKSEMenuFramework.h'),
 ('prisma_ui','PrismaUI-SKSE/framework','aaef0713a6b4c775a654c427871f0fe53b75fcb6','src/PrismaUI_API.h'),
 ('oar','ersh1/OpenAnimationReplacer','f4e7688b065175aff70aa523073857911e15aca3','src/API/OpenAnimationReplacerAPI-Conditions.h'),
]
def fetch(url):
 req = urllib.request.Request(url,headers={'User-Agent':'Skyrim-Template-Cache'})
 with urllib.request.urlopen(req, timeout=180) as response: return response.read()
def one(spec):
 name,repo,ref,sentinel=spec
 if len(ref)==40: revision=ref
 else:
  refs=subprocess.check_output(['git','ls-remote',f'https://github.com/{repo}.git',f'refs/tags/{ref}',f'refs/tags/{ref}^{{}}'],text=True).strip().splitlines()
  if refs: revision=refs[-1].split()[0]
  else: revision=json.loads(fetch(f'https://api.github.com/repos/{repo}/commits/{ref}'))['sha']
 url=f'https://codeload.github.com/{repo}/zip/{revision}'
 archive=CACHE/f'{name}-{revision}.zip'
 if not archive.exists(): archive.write_bytes(fetch(url))
 digest=hashlib.sha256(archive.read_bytes()).hexdigest()
 print(f'{name}: {revision[:12]} ({archive.stat().st_size//1024} KiB)',flush=True)
 return name,dict(repository=f'https://github.com/{repo}',ref=ref,revision=revision,url=url,sha256=digest,required=sentinel)
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 entries=dict(pool.map(one,SPECS))
(ROOT/'dependencies.lock.json').write_text(json.dumps(dict(schema=1,libraries=entries),indent=2)+'\n',encoding='utf-8')
