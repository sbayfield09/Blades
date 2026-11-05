#!/usr/bin/env python3
"""Prepare user-supplied DashX360 art locally. No upstream media is bundled.
Requires Pillow. Optionally merges a local shrdres.xzp to preserve stock logos.
"""
import argparse, io, math, re, struct
from pathlib import Path
import xml.etree.ElementTree as ET
from PIL import Image, ImageChops, ImageFilter
W,H=1280,720
STATES=['marketplace','xbox live','games','media','system']
PREFIX=['Marketplace','XboxLive','Games','Media','System']
COLORS=[(206,118,59),(199,165,94),(100,195,44),(37,140,221),(128,98,213)]
ORIGINS=[140,195,250,294,335]


def unpack_shared(path):
    d=path.read_bytes()
    if len(d)<22 or d[:4]!=b'XUIZ' or struct.unpack_from('>I',d,4)[0]!=1:raise ValueError('unsupported shared archive')
    total,flags,table=struct.unpack_from('>III',d,8);base=22+table
    if total!=len(d) or flags or base>len(d):raise ValueError('invalid shared archive header')
    count=struct.unpack_from('>H',d,20)[0];p=22;out={}
    for _ in range(count):
        if p+9>base:raise ValueError('truncated shared resource table')
        size,offset,n=struct.unpack_from('>IIB',d,p);p+=9
        if p+n*2>base or offset+size>len(d)-base:raise ValueError('invalid shared resource range')
        name=d[p:p+n*2].decode('utf-16-be').rstrip('\0');p+=n*2
        out[name]=d[base+offset:base+offset+size]
    if p!=base:raise ValueError('invalid shared table length')
    return out


def polygon(path):
    # WPF absolute M/L/C/Z geometry, sampled into a polygon. No WPF runtime/code is used.
    tokens=re.findall(r'[MLCZ]|[-+]?(?:\d*\.\d+|\d+)(?:[eE][-+]?\d+)?',path)
    points=[];p=0;current=(0,0)
    while p<len(tokens):
        op=tokens[p];p+=1
        if op in ('M','L'):
            current=(float(tokens[p]),float(tokens[p+1]));p+=2;points.append(current)
        elif op=='C':
            values=list(map(float,tokens[p:p+6]));p+=6
            a=current;b=values[:2];c=values[2:4];d=values[4:]
            for step in range(1,49):
                t=step/48;s=1-t
                points.append((s*s*s*a[0]+3*s*s*t*b[0]+3*s*t*t*c[0]+t*t*t*d[0],s*s*s*a[1]+3*s*s*t*b[1]+3*s*t*t*c[1]+t*t*t*d[1]))
            current=tuple(d)
        elif op=='Z':break
        else:raise ValueError('unsupported geometry command '+op)
    return [(x*W/1920,y*H/1080) for x,y in points]


def row_bounds(points):
    rows=[]
    for y in range(H):
        scan=min(max(y+0.5,min(p[1] for p in points)+1e-4),max(p[1] for p in points)-1e-4);hits=[]
        for a,b in zip(points,points[1:]+points[:1]):
            if (a[1]<=scan<b[1]) or (b[1]<=scan<a[1]):hits.append(a[0]+(scan-a[1])*(b[0]-a[0])/(b[1]-a[1]))
        if len(hits)<2:raise ValueError('body geometry has an empty scan line')
        left=max(0,min(W-1,int(math.floor(min(hits)))));right=max(left+1,min(W,int(math.ceil(max(hits)))))
        rows.append((left,right))
    return rows


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source',required=True,type=Path,help='local DashX360 checkout')
    ap.add_argument('--shared',type=Path,help='optional user-supplied retail shrdres.xzp')
    ap.add_argument('--output',required=True,type=Path,help='local Assets/blades.xzp destination')
    args=ap.parse_args();root=args.source;art=root/'Assets/Blades'
    entries=unpack_shared(args.shared) if args.shared else {}
    rects=[]
    def add_image(state,slot,path,mask=None):
        im=Image.open(path).convert('RGBA').resize((W,H),Image.Resampling.LANCZOS)
        if mask:
            rgba=Image.new('RGBA',(W,H));pixels=rgba.load();alpha=im.getchannel('A');bounds=alpha.getbbox()
            if not bounds:raise ValueError('empty mask')
            edge=ImageChops.subtract(alpha,alpha.filter(ImageFilter.MinFilter(5))) if mask in ('tray','circle') else None
            for y in range(bounds[1],bounds[3]):
                t=(y-bounds[1])/max(1,bounds[3]-bounds[1]-1)
                if mask=='top':color=tuple(int(v*(0.27+0.31*t)) for v in COLORS[state]);opacity=220
                elif mask=='bottom':color=tuple(int(v*0.7+180*0.3) for v in COLORS[state]);opacity=75
                elif mask=='icon':color=tuple(int(v*0.35) for v in COLORS[state]);opacity=110
                else:color=tuple(int(v*0.6) for v in COLORS[state]);opacity=45
                for x in range(bounds[0],bounds[2]):
                    a=alpha.getpixel((x,y))*opacity//255
                    if edge is not None:a=min(255,a+edge.getpixel((x,y))*80//255)
                    pixels[x,y]=(*color,a)
            im=rgba
        bbox=im.getbbox()
        if not bbox:raise ValueError(f'empty art: {path}')
        x,y,right,bottom=bbox;im=im.crop(bbox)
        if max(im.size)>1024:raise ValueError(f'cropped resource exceeds decoder limit: {path} {im.size}')
        payload=io.BytesIO();im.save(payload,format='PNG')
        if slot>=9:x+=ORIGINS[state]-ORIGINS[2]
        if x<0 or x+im.width>W:raise ValueError('tray placement exceeds design bounds')
        entries[f'skin/{state}/{slot}.png']=payload.getvalue();rects.append((x,y,im.width,im.height))
    for state,label in enumerate(STATES):
        folder=art/'Separated'/f'{label} focused'
        for slot,blade in enumerate(STATES):
            path=folder/f'{blade} blade.png'
            if not path.exists() and blade=='marketplace':path=folder/'marketplace.png'
            add_image(state,slot,path)
        add_image(state,5,folder/f'{label} focused left runner.png')
        add_image(state,6,folder/f'{label} focused right runner.png')
        add_image(state,7,art/'FocusedOverlays'/f'{PREFIX[state]}FocusedTop.png','top')
        add_image(state,8,art/'FocusedOverlays'/f'{PREFIX[state]}FocusedBottom.png','bottom')
        for slot,name in enumerate(('bar','circle','icon'),9):add_image(state,slot,art/'OpenTray'/f'open tray {name}.png',{'bar':'tray','circle':'circle','icon':'icon'}[name])
    xml=ET.parse(root/'XboxMetroLauncher/XboxMetroLauncher.Themes/DashX360_CenterBladeBodies.xaml').getroot()
    geometries={node.attrib.get('{http://schemas.microsoft.com/winfx/2006/xaml}Key'):node.text for node in xml if node.tag.endswith('Geometry')}
    geometry_names=['Marketplace','XboxLIVE','Games','Media','System']
    manifest=bytearray(struct.pack('>4s4H',b'BSK1',W,H,5,12))
    for rect in rects:manifest.extend(struct.pack('>4H',*rect))
    for label in geometry_names:
        for left,right in row_bounds(polygon(geometries[label+'CenterBodyGeometry'])):manifest.extend(struct.pack('>2H',left,right))
    entries['skin/layout.bin']=manifest
    # Additional menu icons that were absent from shrdres.xzp.
    icon_paths={
        (0,0):'marketplace/game store.png',(0,1):'marketplace/video store.png',(0,2):'marketplace/account management.png',
        (1,0):'xbox live/ico_32x_Mail.png',(1,1):'xbox live/Friends.png',(1,2):'xbox live/chat and im.png',
        (2,0):'games/Achievements.png',(2,1):'games/Played Games.png',
        (3,0):'media/music.png',(3,1):'media/pictures.png',(3,2):'media/videos.png',(3,3):'media/video store.png',(3,4):'media/media center.png',
        (4,0):'system/console settings.png',(4,1):'system/family settings.png',(4,2):'system/memory.png',(4,3):'system/network settings.png',(4,5):'system/initial setup.png'}
    for (blade,row),name in icon_paths.items():
        im=Image.open(art/'ProvidedIconsFit'/name).convert('RGBA');bounds=im.getbbox()
        if not bounds:continue
        im=im.crop(bounds);im.thumbnail((64,64),Image.Resampling.LANCZOS)
        out=io.BytesIO();im.save(out,format='PNG');entries[f'skin/icon/{blade}/{row}.png']=out.getvalue()
    table=bytearray();payload=bytearray()
    for name,data in sorted(entries.items()):
        raw=name.encode('utf-16-be');chars=len(raw)//2
        if not 0<chars<160:raise ValueError('unsupported resource name')
        table.extend(struct.pack('>IIB',len(data),len(payload),chars)+raw);payload.extend(data)
    total=22+len(table)+len(payload)
    if len(entries)>512 or total>16*1024*1024:raise ValueError('pack exceeds runtime limits')
    result=struct.pack('>4s4IH',b'XUIZ',1,total,0,len(table),len(entries))+table+payload
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(result)
    print(f'Created local pack: {args.output} ({len(entries)} entries, {len(result)} bytes)')
    print('Credit: zivvoz / DashX360, inspired by Microsoft Xbox 360. Upstream media remains user-supplied.')


if __name__=='__main__':main()
