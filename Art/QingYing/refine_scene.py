import bpy, bmesh, math, random
from mathutils import Vector, Matrix, noise
from pathlib import Path

# 在原文件上另存精修版，原版本可随时恢复。
random.seed(9072)
scene = bpy.context.scene
out = Path('D:/ue project/RTS/Art/QingYing')
detail = bpy.data.collections.new('Refinement - stone foliage atmosphere')
scene.collection.children.link(detail)
def material(name, color, scale=0, contrast=.18):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    n=m.node_tree.nodes; l=m.node_tree.links; p=n.get('Principled BSDF'); p.inputs['Base Color'].default_value=(*color,1); p.inputs['Roughness'].default_value=.94
    if scale:
        pos=n.new('ShaderNodeNewGeometry'); tex=n.new('ShaderNodeTexNoise'); tex.inputs['Scale'].default_value=scale; tex.inputs['Detail'].default_value=3.5; tex.inputs['Roughness'].default_value=.72; l.new(pos.outputs['Position'],tex.inputs['Vector'])
        ramp=n.new('ShaderNodeValToRGB'); ramp.color_ramp.elements[0].position=.27; ramp.color_ramp.elements[0].color=(*(v*(1-contrast) for v in color),1); ramp.color_ramp.elements[1].position=.75; ramp.color_ramp.elements[1].color=(*(min(1,v*(1+contrast)) for v in color),1)
        l.new(tex.outputs['Fac'],ramp.inputs[0]); l.new(ramp.outputs[0],p.inputs['Base Color'])
        bump=n.new('ShaderNodeBump'); bump.inputs['Strength'].default_value=.25; bump.inputs['Distance'].default_value=.09; l.new(tex.outputs['Fac'],bump.inputs['Height']); l.new(bump.outputs[0],p.inputs['Normal'])
    return m
chalk=material('Refined - aged warm chalk',(.79,.79,.65),3,.18)
edge=material('Refined - pale chipped limestone',(.86,.83,.68),7,.12)
recess=material('Refined - shadowed openings',(.025,.048,.045))
timber=material('Refined - weathered walnut',(.16,.14,.09),5)
tiles=[material('Refined - roof tile %d'%i,c,4) for i,c in enumerate([(.38,.16,.11),(.47,.23,.16),(.56,.28,.19)])]
cliffs=[material('Refined - mineral layer %d'%i,c,2.2,.22) for i,c in enumerate([(.25,.35,.34),(.29,.40,.38),(.34,.45,.42),(.43,.52,.46),(.53,.60,.51)])]
greens=[material('Refined - foliage %d'%i,c) for i,c in enumerate([(.035,.095,.07),(.055,.15,.085),(.10,.24,.075),(.17,.32,.075),(.29,.43,.09),(.38,.48,.12)])]
meadow=material('Refined - living meadow',(.32,.45,.105),.52,.5)
pathmat=material('Refined - warm worn path',(.41,.40,.25),3,.22)
flowercolors=[material('Refined - petal %d'%i,c) for i,c in enumerate([(.81,.86,.75),(.30,.58,.76),(.93,.70,.12),(.75,.29,.18)])]
def mesh(name,vs,fs,mats,ids=None):
    me=bpy.data.meshes.new(name); me.from_pydata(vs,[],fs); me.update(); ob=bpy.data.objects.new(name,me); detail.objects.link(ob)
    for m in mats:me.materials.append(m)
    if ids is not None:
        for p,i in zip(me.polygons,ids):p.material_index=i
    return ob
def primitive(name,loc,scale,mat,kind='ico',sub=1):
    bm=bmesh.new()
    if kind=='cube': bmesh.ops.create_cube(bm,size=1)
    else:bmesh.ops.create_icosphere(bm,subdivisions=sub,radius=1)
    me=bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); ob=bpy.data.objects.new(name,me); detail.objects.link(ob); ob.location=loc; ob.scale=scale; me.materials.append(mat); return ob
def cube(name,loc,scale,mat):return primitive(name,loc,scale,mat,'cube')
def branch(name,points,radius,mat):
    cu=bpy.data.curves.new(name,'CURVE'); cu.dimensions='3D'; cu.resolution_u=1; cu.bevel_depth=radius; cu.resolution_u=1; cu.bevel_resolution=0; cu.resolution_u=1
    sp=cu.splines.new('POLY'); sp.points.add(len(points)-1)
    for p,v in zip(sp.points,points):p.co=(*v,1)
    ob=bpy.data.objects.new(name,cu); detail.objects.link(ob); cu.materials.append(mat); return ob
def hide(ob):ob.hide_render=True; ob.hide_set(True)
def height(x,y):return .34*x+.16*y+3+.5*math.sin(x*.22+y*.23)+.20*math.sin(x*.7-y*.4)
def n3(x,y,z):return noise.noise_vector(Vector((x,y,z)))[0]

# 保留旧几何作为隐藏的构图底稿。
for ob in list(scene.objects):
    if ob.name.startswith(('Sculpted cumulus','Joined cumulus','Trailing roof ivy','Angular leaf crown','Tree branch','Twisted trunk','Wind swept grass','Wildflower field')):hide(ob)
    if ob.name.startswith(('Great shaded escarpment','Castle grass mountain','Distant stepped chalk')):hide(ob)
    if ob.name.startswith('Weathered masonry patch'):
        ob.data.materials.clear(); ob.data.materials.append(random.choice([chalk,chalk,edge]))
    if ob.type=='MESH' and ob.name in ['Lower gatehouse','Lower narrow tower','Long west facade','Middle high tower','Upper chalk keep','Slender bell tower','East stepped house','Gate annex']:
        ob.data.materials.clear();ob.data.materials.append(chalk)
ground=bpy.data.objects['Rolling sunlit meadow']; ground.data.materials.clear(); ground.data.materials.append(meadow)
for p in ground.data.polygons:p.material_index=0; p.use_smooth=True

# 不规则竖向岩脊、局部断层与侵蚀纹理。
def rockwall(name,x0,x1,y0,bottom,top,steps=110,rows=100):
    vs=[];fs=[];ids=[]
    for j in range(rows+1):
        z=bottom+(top-bottom)*j/rows
        for i in range(steps+1):
            x=x0+(x1-x0)*i/steps
            fault=.65*math.sin(x*2.1)+.23*math.sin(x*5.7)
            y=y0+fault+1.3*n3(x*.32,z*.11,1)+.26*n3(x*2.1,z*.8,2)+.5*math.sin(z*.33+x*.2)
            xx=x+.1*n3(x*1.7,z*.5,5)
            vs.append((xx,y,z+.16*n3(x*1.6,z*.9,7)))
    for j in range(rows):
        for i in range(steps):
            a=j*(steps+1)+i;fs.append((a,a+1,a+steps+2,a+steps+1))
            value=n3(i*.04,j*.07,1)+.25*math.sin(i*.8)
            ids.append(max(0,min(4,int(2+value*2))))
    return mesh(name,vs,fs,cliffs,ids)
rockwall('Eroded towering escarpment',12.8,48,10.4,-6,54)
rockwall('Castle weathered rock foundation',-3.7,11.8,9.3,-12,22.2,60,70)
# 远景平台改为不等宽的破碎山脊。
for k in range(5):
    xa=-30+k*5.7; xb=xa+7; top=-.8+k*1.45
    rockwall('Distant cliff ridge %02d'%k,xa,xb,12+k*.12,-17,top,25,30)
    vs=[];fs=[]
    for j in range(3):
        for i in range(19):
            x=xa+(xb-xa)*i/18; y=11.5+j*4+.3*math.sin(i*.8); vs.append((x,y,top+.2+.18*math.sin(i*.5)))
    for j in range(2):
        for i in range(18):a=j*19+i;fs.append((a,a+1,a+20,a+19))
    mesh('Irregular distant turf',vs,fs,[meadow])
# 山顶草皮顺岩沿下垂，形成原图的绿色山体。
vs=[];fs=[]
for j in range(17):
    for i in range(51):
        x=-3.7+i*.30; y=8.7+j*.47; z=22.3+.45*math.sin(x*.3)+j*.10+.18*n3(x,y,2);vs.append((x,y,z))
for j in range(16):
    for i in range(50):a=j*51+i;fs.append((a,a+1,a+52,a+51))
mesh('Uneven castle plateau meadow',vs,fs,[meadow])

# 岩石薄片与裂缝，方向保持一致但长度不重复。
for k in range(270):
    x=random.uniform(13.2,46); z=random.uniform(7,48); y=9.2+.5*math.sin(x*2.1)
    w=random.uniform(.12,.65); h=random.uniform(.35,2.8)
    vs=[(x-w,y,z-h),(x+w*.7,y+.18,z-h*.7),(x+w,y+.23,z+h*.7),(x+.2*w,y-.18,z+h),(x-w*.55,y-.35,z+.3*h)]
    mesh('Layered cliff flake',vs,[(0,1,4),(1,2,4),(2,3,4)],[random.choice(cliffs)])
for k in range(85):
    x=random.uniform(13,43);z=random.uniform(8,42);y=8.75+.5*math.sin(x*2.1)
    branch('Rock fissure',[(x,y,z),(x+.11,y-.025,z-.45),(x-.08,y,z-1.15)],random.uniform(.008,.021),cliffs[0])

# 石拱窗、门廊、支架与露台。
def arch(name,x,y,z,w,h,depth=.15):
    radius=w/2; spring=z+h-radius;vs=[(x-radius,y,z),(x+radius,y,z)]
    for k in range(13):
        a=k*math.pi/12;vs.append((x+radius*math.cos(a),y,spring+radius*math.sin(a)))
    mesh(name+' dark interior',vs,[tuple(range(len(vs)))],[recess])
    for k in range(11):
        a0=k*math.pi/11; a1=(k+1)*math.pi/11; r0=radius; r1=radius+.10
        v=[]
        for yy in [y-.04,y-depth]:
            v.extend([(x+r0*math.cos(a0),yy,spring+r0*math.sin(a0)),(x+r1*math.cos(a0),yy,spring+r1*math.sin(a0)),(x+r1*math.cos(a1),yy,spring+r1*math.sin(a1)),(x+r0*math.cos(a1),yy,spring+r0*math.sin(a1))])
        mesh(name+' voussoir',v,[(0,1,2,3),(4,7,6,5),(0,4,5,1),(3,2,6,7)],[edge])
    for dx in [-radius-.05,radius+.05]:cube(name+' jamb',(x+dx,y-depth*.6,(z+spring)/2),(.10,depth,spring-z),edge)
    cube(name+' sill',(x,y-.13,z),(w+.30,.3,.1),edge)
for x,y,z in [(6.15,5.72,11),(8.6,5.72,11),(6.15,5.72,17),(8.6,5.72,17),(6.15,5.72,24.5),(8.6,5.72,24.5),(3.4,3.31,9),(3.4,3.31,15.4),(-2,5.41,13.2),(-2,5.41,18.8)]:arch('Carved arched window',x,y,z,.49,1.03)
arch('Sanctuary entrance',-3,1.31,.6,1.24,2.25,.28)
for x,y,z,w in [(7.4,5.3,14.2,4.8),(3.4,2.9,8.5,2.1),(-2,5.1,16.5,2.35)]:
    cube('Balcony stone slab',(x,y,z),(w,1.0,.18),edge)
    branch('Balcony timber handrail',[(x-w/2,y-.45,z+.72),(x+w/2,y-.45,z+.72)],.045,timber)
    for k in range(int(w/.35)+1):
        xx=x-w/2+k*w/int(w/.35);branch('Balcony spindle',[(xx,y-.45,z+.12),(xx,y-.45,z+.72)],.025,timber)
    for dx in [-w*.35,w*.35]:branch('Balcony angled bracket',[(x+dx,y+.35,z-.75),(x+dx,y-.4,z-.1)],.07,timber)
# 小型廊桥连接错层建筑。
cube('Connecting gallery floor',(.75,5.3,10.4),(3.1,1.2,.20),edge)
for x in [-.6,.1,.8,1.5,2.2]:
    cube('Gallery upright',(x,4.78,11.2),(.11,.11,1.5),timber)
cube('Gallery roof',(.8,5.3,12),(3.6,1.6,.18),tiles[1])
# 屋顶瓦片具有独立起伏，遮盖旧平板的正面边缘。
for x,y,z,w,d in [(-3,3,5.016,3.8,3),(-.6,1.8,5.808,2.2,3),(3.4,5,22.264,2.3,3),(10.1,5.8,21.384,2.8,3)]:
    for j in range(7):
        for i in range(int(w/.23)):
            xx=x-w/2+.12+i*.23;yy=y-d/2+.22+j*.40
            o=cube('Individual clay roof tile',(xx,yy,z+.12+random.uniform(0,.025)),(.215,.43,.045),random.choice(tiles));o.rotation_euler.x=.11

# 贴墙藤蔓由枝条和成片叶面组成。
lv=[];lf=[];li=[]
def leafpatch(x,y,z,size,idx,vertical=False):
    a=len(lv);ang=random.random()*math.tau
    for k in range(6):
        t=ang+k*math.tau/6; r=size*random.uniform(.7,1.15)
        lv.append((x+math.cos(t)*r,y+(.08*math.sin(t) if vertical else math.sin(t)*r),z+(math.sin(t)*r if vertical else .06*math.sin(t))))
    lv.append((x,y-.045,z+.035));lf.extend([(a+k,a+(k+1)%6,a+6) for k in range(6)]);li.extend([idx]*6)
for x0,y0,top,w,drop in [(7.4,5.68,30.05,5.4,8.0),(-2,5.36,21.2,2.4,4),(3.4,3.27,22.0,2.1,4),(10.1,4.22,21.1,2.6,5),(.3,8.1,22.5,6.5,15)]:
    for k in range(int(w*5)):
        x=x0+random.uniform(-w/2,w/2); length=random.uniform(.5,drop);points=[]
        for j in range(int(length/.12)):
            z=top-j*.12;xx=x+.16*math.sin(j*.28+k);yy=y0-.05*math.sin(j*.4);points.append((xx,yy,z))
            for side in [-1,1]:leafpatch(xx+side*.12,yy-.03,z,random.uniform(.10,.23),random.randrange(2,6),True)
        if len(points)>1:branch('Climbing vine stem',points,.012,greens[1])
mesh('Cascading wall ivy leaves',lv,lf,greens,li)

# 枝干分叉、细碎叶簇和前景树冠。
folv=[];folf=[];foli=[]
def foliage(center,radius,count):
    cx,cy,cz=center
    for k in range(count):
        a=random.random()*math.tau; t=random.uniform(-1,1); r=radius*random.random()**.35
        x=cx+math.cos(a)*math.sqrt(1-t*t)*r; y=cy+math.sin(a)*math.sqrt(1-t*t)*r*.75; z=cz+t*r*.65
        size=random.uniform(.07,.20);index=len(folv)
        folv.extend([(x-size,y,z),(x,y-size*.65,z+.04),(x+size,y,z+.025),(x,y+size*.65,z),(x,y,z+.1)])
        folf.extend([(index,index+1,index+4),(index+1,index+2,index+4),(index+2,index+3,index+4),(index+3,index,index+4)]);foli.extend([random.choices(range(6),[3,5,5,3,1,1])[0]]*4)
def tree(x,y,s):
    z=height(x,y);p0=(x,y,z);p1=(x-.15*s,y,z+1.6*s);p2=(x+.17*s,y,z+3.0*s)
    branch('Gnarled tree trunk',[p0,p1,p2],.14*s,timber)
    for k in range(11):
        a=k*2.4;r=random.uniform(.7,1.65)*s;tip=(x+math.cos(a)*r,y+math.sin(a)*r*.6,z+random.uniform(2.4,3.7)*s)
        mid=(x+math.cos(a)*r*.4,y+math.sin(a)*r*.25,z+2.0*s)
        branch('Fine tree limb',[p1,mid,tip],.045*s,timber)
        for j in range(3):
            end=(tip[0]+random.uniform(-.7,.7)*s,tip[1]+random.uniform(-.45,.45)*s,tip[2]+random.uniform(-.3,.4)*s)
            branch('Tree twig',[mid,tip,end],.013*s,timber);foliage(end,.65*s,int(150*s))
tree(32,-16,2.0);tree(24,-5,1.0);tree(20,1, .75);tree(38,-8,1.5)
for k in range(34):
    x=random.uniform(-10,36);y=random.uniform(-13,8);foliage((x,y,height(x,y)+.25),random.uniform(.25,.6),95)
mesh('Fine tree and shrub foliage',folv,folf,greens,foli)

# 连续草丛沿坡面分布，并为前景加入更大的花束与石块。
gv=[];gf=[];gi=[];fv=[];ff=[];fi=[]
for k in range(40000):
    x=random.uniform(-23,43);y=random.uniform(-34,12);z=height(x,y)+.03
    if n3(x*.28,y*.25,8)<-.35:continue
    h=random.uniform(.12,.45);w=random.uniform(.014,.032);idx=len(gv);lean=random.uniform(.02,.16)
    gv.extend([(x-w,y,z),(x+w,y,z),(x+w*.3+lean,y+.02,z+h*.65),(x+lean,y+.04,z+h)])
    gf.extend([(idx,idx+1,idx+2),(idx,idx+2,idx+3)]);gi.extend([random.randrange(2,6)]*2)
    if k%9==0:
        color=random.choices(range(4),[5,4,3,1])[0];r=random.uniform(.035,.065);zc=z+h
        for j in range(5):
            a=j*math.tau/5;idx=len(fv);cx=x+math.cos(a)*r;cy=y+math.sin(a)*r
            fv.extend([(x,y,zc),(cx-math.sin(a)*r*.6,cy+math.cos(a)*r*.6,zc+.035),(x+math.cos(a)*r*1.8,y+math.sin(a)*r*1.8,zc+.015),(cx+math.sin(a)*r*.6,cy-math.cos(a)*r*.6,zc+.035)])
            ff.append((idx,idx+1,idx+2,idx+3));fi.append(color)
mesh('Dense directional meadow grasses',gv,gf,greens,gi);mesh('Five petal alpine flowers',fv,ff,flowercolors,fi)
# 曲折小路与破损石阶引向城门。
vs=[];fs=[]
for i in range(95):
    t=i/94;x=-3+22*t;y=.5-17*t+1.5*math.sin(t*8);w=.32+.34*t
    for d in [-1,1]:vs.append((x,y+d*w,height(x,y+d*w)+.035))
for i in range(94):a=i*2;fs.append((a,a+1,a+3,a+2))
mesh('Narrow winding pilgrim trail',vs,fs,[pathmat])
for k in range(62):
    x=random.uniform(-15,36);y=random.uniform(-26,7);r=random.uniform(.25,.9)
    o=primitive('Weathered limestone boulder',(x,y,height(x,y)+r*.15),(r*1.5,r*.8,r*.6),random.choice(cliffs[2:]),sub=2)
    for v in o.data.vertices:v.co*=1+random.uniform(-.18,.18)

# 云朵融合为连续表面，再添加多尺度起伏，消除球体交线。
bm=bmesh.new()
for cx,cz,ss in [(-35,-1,5),(-29,5,5.5),(-25,11,5.0),(-23,16,4.8),(-18,14,4.8),(-13,14,4.5),(-8,15,4.5),(0,34,5.5),(6,36,4.8)]:
    for j in range(17):
        r=random.uniform(.38,.75)*ss;loc=Vector((cx+random.uniform(-ss*.75,ss*.75),27+random.uniform(-2,2),cz+random.uniform(-ss*.7,ss*.7)))
        verts=bmesh.ops.create_icosphere(bm,subdivisions=2,radius=1)['verts']
        for v in verts:v.co=Vector((v.co.x*r,v.co.y*r*.7,v.co.z*r*.86))+loc
me=bpy.data.meshes.new('Continuous cumulus source');bm.to_mesh(me);bm.free()
cloudob=bpy.data.objects.new('Continuous sculpted cloud sea',me);detail.objects.link(cloudob)
cloudmat=material('Refined - soft cumulus',(.88,.92,.89),.9,.07);cloudmat.node_tree.nodes.get('Principled BSDF').inputs['Subsurface Weight'].default_value=.08;me.materials.append(cloudmat)
for ob in bpy.context.selected_objects:ob.select_set(False)
cloudob.select_set(True);bpy.context.view_layer.objects.active=cloudob
rem=cloudob.modifiers.new('Fuse overlapping cloud billows','REMESH');rem.mode='VOXEL';rem.voxel_size=.23;rem.use_smooth_shade=True
bpy.ops.object.modifier_apply(modifier=rem.name)
sm=cloudob.modifiers.new('Soften cloud joins','SMOOTH');sm.factor=1.4;sm.iterations=5
tex=bpy.data.textures.new('Cloud turbulent silhouette',type='CLOUDS');tex.noise_scale=.85;tex.noise_depth=2
disp=cloudob.modifiers.new('Cloud fine billows','DISPLACE');disp.texture=tex;disp.strength=.25;disp.mid_level=.5

# 冷色阴影与暖色日光，保留参考构图相机。
sun=bpy.data.objects['Warm upper left sunlight'];sun.data.energy=2.6;sun.data.angle=.10
sun.rotation_euler=(math.radians(42),math.radians(-32),math.radians(-18))
scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=1920;scene.render.resolution_y=762;scene.render.resolution_percentage=60
scene.render.filepath=str(out/'qing_ying_refined_preview')
for area in bpy.context.screen.areas:
    if area.type=='VIEW_3D':area.spaces.active.shading.type='SOLID';area.spaces.active.shading.color_type='MATERIAL';area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.wm.save_as_mainfile(filepath=str(out/'qing_ying_refined.blend'))
result={'file':bpy.data.filepath,'detail_objects':len(detail.objects),'cloud_vertices':len(cloudob.data.vertices)}
