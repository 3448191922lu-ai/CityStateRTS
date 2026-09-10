import bpy, bmesh, math, random
from mathutils import Vector
random.seed(820)
scene=bpy.context.scene
detail=bpy.data.collections['Refinement - stone foliage atmosphere']

# 云海下部增加连续厚度，与已有云团融合。
cloud=bpy.data.objects['Continuous sculpted cloud sea']
bm=bmesh.new();bm.from_mesh(cloud.data)
for cx,cy,cz,r in [(-32,29,-3,8),(-25,29,0,8),(-20,29,3,7),(-16,30,7,6),(-10,30,8,5),(1,29,38,8)]:
    for j in range(7):
        rr=r*random.uniform(.55,.85);p=Vector((cx+random.uniform(-2,2),cy+random.uniform(-1,1),cz+random.uniform(-2,2)))
        verts=bmesh.ops.create_icosphere(bm,subdivisions=2,radius=1)['verts']
        for v in verts:v.co=Vector((v.co.x*rr,v.co.y*rr*.75,v.co.z*rr*.85))+p
bm.to_mesh(cloud.data);bm.free()
for o in bpy.context.selected_objects:o.select_set(False)
cloud.select_set(True);bpy.context.view_layer.objects.active=cloud
rem=cloud.modifiers.new('Cloud bank union','REMESH');rem.mode='VOXEL';rem.voxel_size=.24;rem.use_smooth_shade=True
# 先融合，再保留现有平滑与噪声修改器。
bpy.ops.object.modifier_move_up(modifier=rem.name);bpy.ops.object.modifier_move_up(modifier=rem.name)
bpy.ops.object.modifier_apply(modifier=rem.name)

# 让主塔正面的白色墙体露出，藤蔓集中在屋檐和边缘。
ivy=bpy.data.objects['Cascading wall ivy leaves'];bm=bmesh.new();bm.from_mesh(ivy.data)
cut=[v for v in bm.verts if 5.05<v.co.x<9.15 and v.co.z>24.3 and v.co.y<6]
bmesh.ops.delete(bm,geom=cut,context='VERTS');bm.to_mesh(ivy.data);bm.free()
for o in list(detail.objects):
    if o.name.startswith('Climbing vine stem'):
        p=o.data.splines[0].points[0].co
        if 5.05<p.x<9.15 and p.z>28 and p.y<6:o.hide_render=True;o.hide_set(True)

# 深灰绿的右壁与日照草坡形成明暗层次。
shadowmats=[]
for i in range(5):
    m=bpy.data.materials['Refined - mineral layer %d'%i].copy();m.name='Refined - shaded cliff %d'%i
    ramp=next(n for n in m.node_tree.nodes if n.type=='VALTORGB')
    for e in ramp.color_ramp.elements:
        c=e.color;e.color=(c[0]*.58,c[1]*.65,c[2]*.69,1)
    shadowmats.append(m)
for o in detail.objects:
    if o.name.startswith(('Eroded towering escarpment','Layered cliff flake')):
        o.data.materials.clear()
        for m in shadowmats:o.data.materials.append(m)

# 近景树以不对称分枝和立体叶面覆盖右侧画框。
wood=bpy.data.materials['Refined - weathered walnut']
greens=[bpy.data.materials['Refined - foliage %d'%i] for i in range(6)]
def h(x,y):return .34*x+.16*y+3+.5*math.sin(x*.22+y*.23)+.2*math.sin(x*.7-y*.4)
def branch(name,points,r):
    cu=bpy.data.curves.new(name,'CURVE');cu.dimensions='3D';cu.bevel_depth=r;cu.bevel_resolution=1
    sp=cu.splines.new('POLY');sp.points.add(len(points)-1)
    for p,co in zip(sp.points,points):p.co=(*co,1)
    ob=bpy.data.objects.new(name,cu);detail.objects.link(ob);cu.materials.append(wood)
x=37.5;y=-26;z=h(x,y);branch('Foreground ancient trunk',[(x,y,z),(x-.55,y+.1,z+3),(x-.2,y,z+6),(x-1.2,y,z+9)],.32)
vs=[];fs=[];ids=[]
for k in range(27):
    angle=k*2.4;r=random.uniform(1.6,4.1);zz=z+random.uniform(3.0,9.1)
    end=Vector((x+math.cos(angle)*r,y+math.sin(angle)*r*.55,zz));mid=Vector((x-.4,y,z+2.8))*.4+end*.6
    branch('Foreground spreading limb',[(x-.4,y,z+2.8),mid,end],random.uniform(.045,.095))
    for j in range(1200):
        a=random.random()*math.tau;t=random.uniform(-1,1);rr=1.3*random.random()**.33
        p=end+Vector((math.cos(a)*math.sqrt(1-t*t)*rr,math.sin(a)*math.sqrt(1-t*t)*rr*.8,t*rr*.9))
        sz=random.uniform(.055,.17);v1=Vector((math.cos(a)*sz,math.sin(a)*sz,random.uniform(-.04,.10)));v2=Vector((-math.sin(a)*sz*.6,math.cos(a)*sz*.6,random.uniform(-.05,.08)))
        index=len(vs);vs.extend([p-v1,p+v2,p+v1,p-v2,p+Vector((0,0,.045))]);fs.extend([(index,index+1,index+4),(index+1,index+2,index+4),(index+2,index+3,index+4),(index+3,index,index+4)]);ids.extend([random.choices(range(6),[5,7,3,1,0,0])[0]]*4)
me=bpy.data.meshes.new('Foreground fine leaves');me.from_pydata(vs,[],fs);me.update();ob=bpy.data.objects.new('Foreground dense tree crown',me);detail.objects.link(ob)
for m in greens:me.materials.append(m)
for p,i in zip(me.polygons,ids):p.material_index=i

# 前景旅人向坡上移动，衣袍增加可见的褶皱。
for o in bpy.data.collections['Travelers'].objects:
    if o.name in ['Traveler linen robe','Traveler hood','Traveler blue cloak','Walking staff']:
        delta=h(13,-4)-h(13,-10);o.location.y+=6;o.location.z+=delta
scene.cycles.samples=64
scene.render.resolution_percentage=100
scene.render.filepath='D:/ue project/RTS/Art/QingYing/qing_ying_refined_final'
readme=bpy.data.texts.get('README_场景说明')
readme.write('\n精修版：补充拱窗、阳台、连廊、单片瓦片、垂落藤蔓、侵蚀岩层、融合云海、精细树叶、花草与小路。原版本文件保留，旧构图几何在精修版内隐藏。\n')
bpy.ops.wm.save_as_mainfile(filepath='D:/ue project/RTS/Art/QingYing/qing_ying_refined.blend')
result={'saved':bpy.data.filepath,'cloud_vertices':len(cloud.data.vertices),'foreground_leaf_faces':len(me.polygons)}
