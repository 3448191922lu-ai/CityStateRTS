import bpy, math, random
from mathutils import Vector
from pathlib import Path

random.seed(72)
out = Path('D:/ue project/RTS/Art/QingYing')
# 独立场景保留已有内容。
scene = bpy.data.scenes.new('QingYing_CliffSanctuary')
bpy.context.window.scene = scene
groups = {}
for name in ['Terrain', 'Architecture', 'Vegetation', 'Clouds', 'Travelers', 'Lighting']:
    c = bpy.data.collections.new(name)
    scene.collection.children.link(c)
    groups[name] = c
def move(o, group):
    for c in list(o.users_collection): c.objects.unlink(o)
    groups[group].objects.link(o)
    return o
def mat(name, color, noise=0):
    m = bpy.data.materials.new(name); m.diffuse_color = (*color, 1); m.use_nodes = True
    nt = m.node_tree; p = nt.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value = (*color,1); p.inputs['Roughness'].default_value = .92
    if noise:
        n = nt.nodes.new('ShaderNodeTexNoise'); n.inputs['Scale'].default_value=noise; n.inputs['Detail'].default_value=3
        r = nt.nodes.new('ShaderNodeValToRGB')
        r.color_ramp.elements[0].position=.22; r.color_ramp.elements[0].color=(*(v*.6 for v in color),1)
        r.color_ramp.elements[1].position=.8; r.color_ramp.elements[1].color=(*(min(1,v*1.18) for v in color),1)
        nt.links.new(n.outputs['Fac'],r.inputs[0]); nt.links.new(r.outputs[0],p.inputs['Base Color'])
    return m
stone=mat('Ivory chalk plaster',(.78,.80,.68),5)
trim=mat('Warm limestone edges',(.91,.87,.70),8)
rock=[mat('Cliff slate %02d'%i,c,3) for i,c in enumerate([(.26,.38,.38),(.36,.48,.46),(.52,.60,.55),(.65,.71,.64),(.19,.30,.31)])]
grass=[mat('Meadow %02d'%i,c,4) for i,c in enumerate([(.24,.39,.065),(.40,.57,.095),(.52,.65,.15),(.32,.49,.09),(.16,.30,.075)])]
leaf=[mat('Leaf %02d'%i,c) for i,c in enumerate([(.045,.12,.095),(.07,.19,.11),(.12,.27,.08),(.21,.35,.07)])]
dark=mat('Deep window recess',(.045,.085,.09)); red=mat('Faded terracotta',(.48,.20,.145),6)
wood=mat('Old dark timber',(.14,.20,.16)); cloud=mat('Cloud porcelain',(.91,.95,.91))
blue=mat('Blue traveling cloak',(.035,.16,.22)); cloth=mat('Linen',(.77,.78,.64))
flowers=[mat('Flower %d'%i,c) for i,c in enumerate([(.86,.73,.16),(.72,.85,.9),(.84,.4,.29),(.9,.9,.73)])]
def mesh(name, verts, faces, materials, group, ids=None):
    me=bpy.data.meshes.new(name); me.from_pydata(verts,[],faces); me.update()
    o=bpy.data.objects.new(name,me); groups[group].objects.link(o)
    for m in materials: me.materials.append(m)
    if ids:
        for p,i in zip(me.polygons,ids): p.material_index=i
    return o
def cube(name,loc,scale,material,group='Architecture',bevel=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object; o.name=name; o.scale=scale; o.data.materials.append(material); move(o,group)
    if bevel:
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        b=o.modifiers.new('Soft worn corners','BEVEL'); b.width=bevel; b.segments=1
    return o
def ico(name,loc,scale,material,group='Vegetation',sub=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=sub,radius=1,location=loc)
    o=bpy.context.object; o.name=name; o.scale=scale; o.data.materials.append(material); move(o,group); return o
def rod(name,a,b,r,material,group='Vegetation'):
    d=Vector(b)-Vector(a); bpy.ops.mesh.primitive_cone_add(vertices=7,radius1=r,radius2=r*.62,depth=d.length,location=(Vector(a)+Vector(b))/2)
    o=bpy.context.object; o.name=name; o.rotation_euler=d.to_track_quat('Z','Y').to_euler(); o.data.materials.append(material); move(o,group); return o
def height(x,y): return .34*x+.16*y+3.0+ .5*math.sin(x*.22+y*.23)+.20*math.sin(x*.7-y*.4)
# 草坡与崖面采用真实网格，镜头外保留足够延伸。
v=[]; f=[]; ids=[]; nx=85; ny=34
for j in range(ny):
    y=-22+j*1.15
    for i in range(nx):
        x=-25+i*.85; v.append((x,y,height(x,y)+random.uniform(-.14,.14)))
for j in range(ny-1):
    for i in range(nx-1):
        a=j*nx+i; f.extend([(a,a+1,a+nx+1),(a,a+nx+1,a+nx)]); ids.extend([random.choices(range(5),[2,4,3,3,1])[0]]*2)
mesh('Rolling sunlit meadow',v,f,grass,'Terrain',ids)
def cliff(name,x,y,w,top,bottom,depth,palette):
    vs=[]; fs=[]; mi=[]; count=18; levels=8
    for k in range(levels):
        z=bottom+(top-bottom)*k/(levels-1)
        for i in range(count):
            xx=x-w/2+w*i/(count-1)
            yy=y+random.uniform(-.65,.65)+.45*math.sin(i*2.4)+.12*k
            vs.append((xx,yy,z+random.uniform(-.5,.5)))
    for k in range(levels-1):
        for i in range(count-1):
            a=k*count+i; fs.append((a,a+1,a+count+1,a+count)); mi.append(random.randrange(len(palette)))
    mesh(name,vs,fs,palette,'Terrain',mi)
    cube(name+' mass',(x,y+depth/2+1,(top+bottom)/2),(w,depth,top-bottom),palette[0],'Terrain')
    cube(name+' turf',(x,y+depth/2,top+.1),(w,depth,.45),grass[0],'Terrain')
for i in range(7):
    x=-28+i*4.2; cliff('Distant stepped chalk %02d'%i,x,10,4.8,1+i*1.1,-18,8,rock[1:4])
cliff('Great shaded escarpment',28,12,29,53,-4,14,[rock[0],rock[0],rock[1],rock[4]])
cliff('Castle grass mountain',4,11,14,26,-8,7,[rock[1],rock[2],rock[3]])
# 依山而建的细长白塔，屋顶与露台错层。
def building(name,x,y,base,w,d,h):
    cube(name,(x,y,base+h/2),(w,d,h),stone,bevel=.075)
    for z in [base+.3,base+h*.34,base+h*.68,base+h-.25]:
        cube(name+' stone stringcourse',(x,y-d/2-.04,z),(w+.16,.18,.13),trim)
    cube(name+' roof garden',(x,y,base+h+.06),(w+.18,d+.18,.25),grass[0])
    for side in [-1,1]: cube(name+' parapet',(x+side*w/2,y,base+h+.36),(.14,d,.65),stone)
    for z in range(int(base+1.3),int(base+h-.7),2):
        for dx in [-.25,.25] if w>2 else [0]:
            xx=x+w*dx+random.uniform(-.12,.12); yy=y-d/2-.058
            cube('Inset tall window',(xx,yy,z),(.24,.045,.71),dark)
            cube('Window limestone sill',(xx,yy-.055,z-.38),(.38,.17,.085),trim)
            if random.random()<.46: cube('Rust red shutter',(xx+.17,yy-.03,z),(.15,.08,.64),red)
    for k in range(int(h*w*2)):
        xx=x+random.uniform(-w*.47,w*.47); z=base+random.uniform(.2,h-.2)
        cube('Weathered masonry patch',(xx,y-d/2-.031,z),(random.uniform(.08,.42),.035,random.uniform(.035,.15)),random.choice([trim,rock[2],red,stone]))
    for k in range(int(w*8)):
        xx=x+random.uniform(-w/2,w/2); zz=base+h-random.uniform(0,h*.4)
        ico('Trailing roof ivy',(xx,y-d/2-.1,zz),(random.uniform(.15,.42),.16,random.uniform(.2,.8)),random.choice(leaf[2:]))
building('Lower gatehouse',-3,3,-1,3.7,3.2,6.5)
building('Lower narrow tower',-5.5,5,-2,1.5,2.6,7)
building('Long west facade',-2,7,2,2.4,3,22)
building('Middle high tower',3.4,5,2,2.1,3.2,23)
building('Upper chalk keep',7.4,8,9,5.4,4.4,25)
building('Slender bell tower',4.7,8,17,1.2,1.8,13)
building('East stepped house',10.1,5.8,10,2.6,3,14)
building('Gate annex',-.6,1.8,1,2,2.3,5.5)
for x,y,z,w in [(-3,3,5.7,3.8),(-.6,1.8,6.6,2.2),(3.4,5,25.3,2.3),(10.1,5.8,24.3,2.8)]:
    cube('Terracotta roof',(x,y,z),(w,3,.23),red)
cube('Bell tower dark canopy',(4.7,8,30.4),(1.6,2.2,.35),wood)
for k in range(32):
    x=4.8+ k*.095; y=1.1+k*.13; z=height(x,y)+k*.14
    cube('Winding stair %02d'%k,(x,y,z),(.95,.4,.18),trim)
for k in range(130):
    x=random.uniform(-17,38); y=random.uniform(-18,10); z=height(x,y)
    s=random.uniform(.1,.65); ico('Meadow chalk stone',(x,y,z),(s*1.8,s,s*.55),random.choice(rock[2:4]),'Terrain')
# 合并的草叶和花瓣网格，避免大量独立对象。
gv=[]; gf=[]; gi=[]; fv=[]; ff=[]; fi=[]
for k in range(15000):
    x=random.uniform(-23,43); y=random.uniform(-20,13); z=height(x,y)+.1
    h=random.uniform(.09,.4); w=h*.13; a=len(gv)
    gv.extend([(x-w,y,z),(x+w,y,z),(x+random.uniform(-.12,.12),y,z+h)]); gf.append((a,a+1,a+2)); gi.append(random.randrange(5))
    if k%7==0:
        r=random.uniform(.035,.09); a=len(fv)
        fv.extend([(x-r,y,z+h),(x,y-r,z+h+.04),(x+r,y,z+h),(x,y+r,z+h+.04),(x,y,z+h+.09)])
        ff.extend([(a,a+1,a+4),(a+1,a+2,a+4),(a+2,a+3,a+4),(a+3,a,a+4)]); fi.extend([random.randrange(4)]*4)
mesh('Wind swept grass blades',gv,gf,grass,'Vegetation',gi)
mesh('Wildflower field',fv,ff,flowers,'Vegetation',fi)
def tree(x,y,s):
    z=height(x,y); top=(x+.3*s,y,z+3.6*s); rod('Twisted trunk',(x,y,z),top,.16*s,wood)
    for k in range(12):
        a=random.random()*math.tau; r=random.uniform(.6,1.8)*s
        p=(x+math.cos(a)*r,y+math.sin(a)*r,z+random.uniform(2.5,4.4)*s)
        rod('Tree branch',(x+.15*s,y,z+2*s),p,.065*s,wood)
        ico('Angular leaf crown',p,(1.0*s,.72*s,.65*s),random.choice(leaf[:3]),sub=2)
tree(29,-9,2.1); tree(21,-1,1.05); tree(35,-3,1.4)
def traveler(x,y,s):
    z=height(x,y)
    bpy.ops.mesh.primitive_cone_add(vertices=9,radius1=.27*s,radius2=.14*s,depth=.9*s,location=(x,y,z+.55*s))
    o=bpy.context.object; o.name='Traveler linen robe'; o.data.materials.append(cloth); move(o,'Travelers')
    ico('Traveler hood',(x,y,z+1.16*s),(.17*s,.16*s,.22*s),blue,'Travelers',2)
    mesh('Traveler blue cloak',[(x-.22*s,y-.12*s,z+.96*s),(x+.22*s,y-.12*s,z+.96*s),(x+.3*s,y-.14*s,z+.3*s),(x,y-.24*s,z+.43*s),(x-.3*s,y-.14*s,z+.3*s)],[(0,1,3),(1,2,3),(0,3,4)],[blue],'Travelers')
    rod('Walking staff',(x+.36*s,y,z),(x+.4*s,y,z+.86*s),.02*s,wood,'Travelers')
traveler(13,-10,1.65); traveler(3,-1,.85); traveler(4,-.5,.75)
# 多层立体积云组成斜向云海。
for cx,cz,ss in [(-32,-2,6),(-27,3,6),(-23,8,6),(-20,15,6),(-16,21,5),(-10,17,5),(-3,23,5),(0,37,6),(9,39,5)]:
    for k in range(14):
        x=cx+random.uniform(-ss,ss); z=cz+random.uniform(-ss*.65,ss*.7); y=25+random.uniform(-2,4)
        r=random.uniform(ss*.32,ss*.65)
        o=ico('Sculpted cumulus',(x,y,z),(r,r*.7,r*.95),cloud,'Clouds',3)
        for p in o.data.polygons: p.use_smooth=True
world=bpy.data.worlds.new('Cobalt summer sky'); world.use_nodes=True; world.node_tree.nodes.get('Background').inputs[0].default_value=(.075,.26,.49,1); world.node_tree.nodes.get('Background').inputs[1].default_value=.55; scene.world=world
bpy.ops.object.light_add(type='SUN',location=(-25,-30,50)); sun=bpy.context.object; sun.name='Warm upper left sunlight'; sun.rotation_euler=(math.radians(25),math.radians(-35),math.radians(-25)); sun.data.energy=3; sun.data.angle=.08; move(sun,'Lighting')
bpy.ops.object.camera_add(location=(0,-95,33)); cam=bpy.context.object; cam.name='Reference composition'; cam.rotation_euler=(Vector((0,3,17))-cam.location).to_track_quat('-Z','Y').to_euler(); cam.data.type='ORTHO'; cam.data.ortho_scale=76; scene.camera=cam; move(cam,'Lighting')
scene.render.engine='CYCLES'; scene.cycles.samples=32; scene.cycles.use_denoising=True
scene.render.resolution_x=1920; scene.render.resolution_y=762; scene.render.resolution_percentage=60
scene.view_settings.view_transform='Standard'; scene.view_settings.look='None'; scene.view_settings.exposure=0; scene.view_settings.gamma=1
scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(out/'qing_ying_preview.png')
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D': area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.wm.save_as_mainfile(filepath=str(out/'qing_ying_scene.blend'))
result={'scene':scene.name,'objects':len(scene.objects),'file':bpy.data.filepath}
