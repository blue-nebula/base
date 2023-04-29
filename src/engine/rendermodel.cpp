#include "engine.h"

VAR(0, oqdynent, 0, 1, 1);
VAR(0, animationinterpolationtime, 0, 200, 1000);

model *loadingmodel = NULL;

#include "ragdoll.h"
#include "animmodel.h"
#include "vertmodel.h"
#include "skelmodel.h"

static model *(__cdecl *modeltypes[NUMMODELTYPES])(const char *);

static int addmodeltype(int type, model *(__cdecl *loader)(const char *))
{
    modeltypes[type] = loader;
    return type;
}

#define MODELTYPE(modeltype, modelclass) \
static model *__loadmodel__##modelclass(const char *filename) \
{ \
    return new modelclass(filename); \
} \
UNUSED static int __dummy__##modelclass = addmodeltype((modeltype), __loadmodel__##modelclass);

#include "md3.h"
#include "md5.h"
#include "obj.h"
#include "smd.h"
#include "iqm.h"

MODELTYPE(MDL_MD3, md3);
MODELTYPE(MDL_MD5, md5);
MODELTYPE(MDL_OBJ, obj);
MODELTYPE(MDL_SMD, smd);
MODELTYPE(MDL_IQM, iqm);

#define checkmdl if(!loadingmodel) { conoutf("\frnot loading a model"); return; }

void mdlmaterial(int *material, int *material2)
{
    checkmdl;
    loadingmodel->setmaterial(clamp(*material, 0, int(MAXLIGHTMATERIALS)), clamp(*material2, 0, int(MAXLIGHTMATERIALS)));
}

COMMAND(0, mdlmaterial, "ii");

void mdlcullface(int *cullface)
{
    checkmdl;
    loadingmodel->setcullface(*cullface!=0);
}

COMMAND(0, mdlcullface, "i");

void mdlcollide(int *collide)
{
    checkmdl;
    loadingmodel->collide = *collide!=0;
}

COMMAND(0, mdlcollide, "i");

void mdlellipsecollide(int *collide)
{
    checkmdl;
    loadingmodel->ellipsecollide = *collide!=0;
}

COMMAND(0, mdlellipsecollide, "i");

void mdlspec(int *percent)
{
    checkmdl;
    float spec = 1.0f;
    if(*percent>0) spec = *percent/100.0f;
    else if(*percent<0) spec = 0.0f;
    loadingmodel->setspec(spec);
}

COMMAND(0, mdlspec, "i");

void mdlambient(int *percent)
{
    checkmdl;
    float ambient = 0.3f;
    if(*percent>0) ambient = *percent/100.0f;
    else if(*percent<0) ambient = 0.0f;
    loadingmodel->setambient(ambient);
}

COMMAND(0, mdlambient, "i");

void mdlalphatest(float *cutoff)
{
    checkmdl;
    loadingmodel->setalphatest(max(0.0f, min(1.0f, *cutoff)));
}

COMMAND(0, mdlalphatest, "f");

void mdlalphablend(int *blend)
{
    checkmdl;
    loadingmodel->setalphablend(*blend!=0);
}

COMMAND(0, mdlalphablend, "i");

void mdlalphadepth(int *depth)
{
    checkmdl;
    loadingmodel->alphadepth = *depth!=0;
}

COMMAND(0, mdlalphadepth, "i");

void mdldepthoffset(int *offset)
{
    checkmdl;
    loadingmodel->depthoffset = *offset!=0;
}

COMMAND(0, mdldepthoffset, "i");

void mdlglow(int *percent, int *delta, float *pulse)
{
    checkmdl;
    float glow = 3.0f, glowdelta = *delta/100.0f, glowpulse = *pulse > 0 ? *pulse/1000.0f : 0;
    if(*percent>0) glow = *percent/100.0f;
    else if(*percent<0) glow = 0.0f;
    glowdelta -= glow;
    loadingmodel->setglow(glow, glowdelta, glowpulse);
}

COMMAND(0, mdlglow, "iif");

void mdlglare(float *specglare, float *glowglare)
{
    checkmdl;
    loadingmodel->setglare(*specglare, *glowglare);
}

COMMAND(0, mdlglare, "ff");

void mdlenvmap(float *envmapmax, float *envmapmin, const char *envmap)
{
    checkmdl;
    loadingmodel->setenvmap(*envmapmin, *envmapmax, envmap[0] ? cubemapload(envmap) : NULL);
}

COMMAND(0, mdlenvmap, "ffs");

void mdlfullbright(float *fullbright)
{
    checkmdl;
    loadingmodel->setfullbright(*fullbright);
}

COMMAND(0, mdlfullbright, "f");

void mdlshader(const char *shader)
{
    checkmdl;
    loadingmodel->setshader(lookupshaderbyname(shader));
}

COMMAND(0, mdlshader, "s");

void mdlspin(float *yaw, float *pitch, float *roll)
{
    checkmdl;
    loadingmodel->spinyaw = *yaw;
    loadingmodel->spinpitch = *pitch;
    loadingmodel->spinroll = *roll;
}

COMMAND(0, mdlspin, "fff");

void mdlscale(int *percent)
{
    checkmdl;
    float scale = 1.0f;
    if(*percent>0) scale = *percent/100.0f;
    loadingmodel->scale = scale;
}

COMMAND(0, mdlscale, "i");

void mdlscalef(float *amt)
{
    checkmdl;
    loadingmodel->scale = *amt;
}

COMMAND(0, mdlscalef, "f");

void mdltrans(float *x, float *y, float *z)
{
    checkmdl;
    loadingmodel->translate = vec(*x, *y, *z);
}

COMMAND(0, mdltrans, "fff");

void mdlyaw(float *angle)
{
    checkmdl;
    loadingmodel->offsetyaw = *angle;
}

COMMAND(0, mdlyaw, "f");

void mdlpitch(float *angle)
{
    checkmdl;
    loadingmodel->offsetpitch = *angle;
}

COMMAND(0, mdlpitch, "f");

void mdlroll(float *angle)
{
    checkmdl;
    loadingmodel->offsetroll = *angle;
}

COMMAND(0, mdlroll, "f");

void mdlshadow(int *shadow)
{
    checkmdl;
    loadingmodel->shadow = *shadow!=0;
}

COMMAND(0, mdlshadow, "i");

void mdlbb(float *rad, float *h, float *height)
{
    checkmdl;
    loadingmodel->collidexyradius = *rad;
    loadingmodel->collideheight = *h;
    loadingmodel->height = *height;
}

COMMAND(0, mdlbb, "fff");

void mdlextendbb(float *x, float *y, float *z)
{
    checkmdl;
    loadingmodel->bbextend = vec(*x, *y, *z);
}

COMMAND(0, mdlextendbb, "fff");

void mdlname()
{
    checkmdl;
    result(loadingmodel->name);
}

COMMAND(0, mdlname, "");

#define checkragdoll \
    if(!loadingmodel->skeletal()) { conoutf("\frnot loading a skeletal model"); return; } \
    skelmodel *m = (skelmodel *)loadingmodel; \
    if(m->parts.empty()) return; \
    skelmodel::skelmeshgroup *meshes = (skelmodel::skelmeshgroup *)m->parts.last()->meshes; \
    if(!meshes) return; \
    skelmodel::skeleton *skel = meshes->skel; \
    if(!skel->ragdoll) skel->ragdoll = new ragdollskel; \
    ragdollskel *ragdoll = skel->ragdoll; \
    if(ragdoll->loaded) return;


void rdvert(float *x, float *y, float *z, float *radius)
{
    checkragdoll;
    ragdollskel::vert &v = ragdoll->verts.add();
    v.pos = vec(*x, *y, *z);
    v.radius = *radius > 0 ? *radius : 1;
}
COMMAND(0, rdvert, "ffff");

void rdeye(int *v)
{
    checkragdoll;
    ragdoll->eye = *v;
}
COMMAND(0, rdeye, "i");

void rdtri(int *v1, int *v2, int *v3)
{
    checkragdoll;
    ragdollskel::tri &t = ragdoll->tris.add();
    t.vert[0] = *v1;
    t.vert[1] = *v2;
    t.vert[2] = *v3;
}
COMMAND(0, rdtri, "iii");

void rdjoint(int *n, int *t, int *v1, int *v2, int *v3)
{
    checkragdoll;
    if(*n < 0 || *n >= skel->numbones) return;
    ragdollskel::joint &j = ragdoll->joints.add();
    j.bone = *n;
    j.tri = *t;
    j.vert[0] = *v1;
    j.vert[1] = *v2;
    j.vert[2] = *v3;
}
COMMAND(0, rdjoint, "iibbb");

void rdlimitdist(int *v1, int *v2, float *mindist, float *maxdist)
{
    checkragdoll;
    ragdollskel::distlimit &d = ragdoll->distlimits.add();
    d.vert[0] = *v1;
    d.vert[1] = *v2;
    d.mindist = *mindist;
    d.maxdist = max(*maxdist, *mindist);
}
COMMAND(0, rdlimitdist, "iiff");

void rdlimitrot(int *t1, int *t2, float *maxangle, float *qx, float *qy, float *qz, float *qw)
{
    checkragdoll;
    ragdollskel::rotlimit &r = ragdoll->rotlimits.add();
    r.tri[0] = *t1;
    r.tri[1] = *t2;
    r.maxangle = *maxangle * RAD;
    r.middle = matrix3(quat(*qx, *qy, *qz, *qw));
}
COMMAND(0, rdlimitrot, "iifffff");

void rdanimjoints(int *on)
{
    checkragdoll;
    ragdoll->animjoints = *on!=0;
}
COMMAND(0, rdanimjoints, "i");

// mapmodels

vector<mapmodelinfo> mapmodels;

void mmodel(char *name)
{
    mapmodelinfo &mmi = mapmodels.add();
    copystring(mmi.name, name);
    mmi.m = NULL;
}

void mapmodelcompat(int *rad, int *h, int *tex, char *name, char *shadow)
{
    mmodel(name);
}

void resetmapmodels() { mapmodels.shrink(0); }

mapmodelinfo *getmminfo(int i) { return mapmodels.inrange(i) ? &mapmodels[i] : 0; }

COMMAND(0, mmodel, "s");
COMMANDN(0, mapmodel, mapmodelcompat, "iiiss");
ICOMMAND(0, mapmodelreset, "", (void), if(editmode || identflags&IDF_WORLD) resetmapmodels(););
ICOMMAND(0, mapmodelindex, "s", (char *a), {
    if(!*a) intret(mapmodels.length());
    else
    {
        int num = parseint(a);
        if(mapmodels.inrange(num)) result(mapmodels[num].name);
    }
});

// model registry

hashnameset<model *> models;
vector<const char *> preloadmodels;

void preloadmodel(const char *name)
{
    if(!name || !name[0] || models.access(name)) return;
    preloadmodels.add(newstring(name));
}

void flushpreloadedmodels(bool msg)
{
    loopv(preloadmodels)
    {
        loadprogress = float(i+1)/preloadmodels.length();
        model *m = loadmodel(preloadmodels[i], -1, msg);
        if(!m) { if(msg) conoutf("\frcould not load model: %s", preloadmodels[i]); }
        else
        {
            m->preloadmeshes();
        }
    }
    preloadmodels.deletearrays();
    loadprogress = 0;
}

void preloadusedmapmodels(bool msg, bool bih)
{
    vector<int> mapmodels;
    vector<extentity *> &ents = entities::getents();
    loopv(ents)
    {
        extentity &e = *ents[i];
        if(e.type == ET_MAPMODEL && e.attrs[0] >= 0)
        {
            if(mapmodels.find(e.attrs[0]) < 0) mapmodels.add(e.attrs[0]);
        }
    }

    loopv(mapmodels)
    {
        loadprogress = float(i+1)/mapmodels.length();
        int mmindex = mapmodels[i];
        mapmodelinfo *mmi = getmminfo(mmindex);
        if(!mmi) { if(msg) conoutf("\frcould not find map model: %d", mmindex); }
        else if(!loadmodel(NULL, mmindex, true)) { if(msg) conoutf("\frcould not load model: %s", mmi->name); }
        else if(mmi->m)
        {
            if(bih) mmi->m->preloadBIH();
            mmi->m->preloadmeshes();
        }
    }
    loadprogress = 0;
}

model *loadmodel(const char *name, int i, bool msg)
{
    if(!name)
    {
        if(!mapmodels.inrange(i)) return NULL;
        mapmodelinfo &mmi = mapmodels[i];
        if(mmi.m) return mmi.m;
        name = mmi.name;
    }
    model **mm = models.access(name);
    model *m;
    if(mm) m = *mm;
    else
    {
        if(loadingmodel || lightmapping > 1) return NULL;
        if(msg)
        {
            defformatstring(str, "%s", name);
            progress(loadprogress, str);
        }
        loopi(NUMMODELTYPES)
        {
            m = modeltypes[i](name);
            if(!m) continue;
            loadingmodel = m;
            if(m->load()) break;
            DELETEP(m);
        }
        loadingmodel = NULL;
        if(!m) return NULL;
        models.access(m->name, m);
        m->preloadshaders();
    }
    if(mapmodels.inrange(i) && !mapmodels[i].m) mapmodels[i].m = m;
    return m;
}

void preloadmodelshaders(bool force)
{
    if(initing) return;
    enumerate(models, model *, m, m->preloadshaders(force));
}

void clear_mdls()
{
    enumerate(models, model *, m, delete m);
}

void cleanupmodels()
{
    enumerate(models, model *, m, m->cleanup());
}

void clearmodel(char *name)
{
    model **m = models.access(name);
    if(!m) { conoutf("\frmodel %s is not loaded", name); return; }
    loopv(mapmodels) if(mapmodels[i].m==*m) mapmodels[i].m = NULL;
    models.remove(name);
    (*m)->cleanup();
    delete *m;
    conoutf("\fgcleared model %s", name);
}

COMMAND(0, clearmodel, "s");

bool modeloccluded(const vec &center, float radius)
{
    ivec bbmin(vec(center).sub(radius)), bbmax(vec(center).add(radius+1));
    return pvsoccluded(bbmin, bbmax) || bboccluded(bbmin, bbmax);
}

VAR(0, showboundingbox, 0, 0, 2);

void render2dbox(vec &o, float x, float y, float z, const matrix4x3 *m)
{
    vec v[4] = { o, vec(o.x, o.y, o.z+z), vec(o.x+x, o.y+y, o.z+z), vec(o.x+x, o.y+y, o.z) };
    gle::begin(GL_LINE_LOOP);
    if(m) loopk(4) gle::attrib(m->transform(v[k]));
    else loopk(4) gle::attrib(v[k]);
    xtraverts += gle::end();
}

void render3dbox(vec &o, float tofloor, float toceil, float xradius, float yradius, const matrix4x3 *m)
{
    if(yradius<=0) yradius = xradius;
    vec c = o;
    c.sub(vec(xradius, yradius, tofloor));
    float xsz = xradius*2, ysz = yradius*2;
    float h = tofloor+toceil;
    gle::colorf(1, 1, 1);
    gle::defvertex();
    render2dbox(c, xsz, 0, h, m);
    render2dbox(c, 0, ysz, h, m);
    c.add(vec(xsz, ysz, 0));
    render2dbox(c, -xsz, 0, h, m);
    render2dbox(c, 0, -ysz, h, m);
}

void renderellipse(vec &o, float xradius, float yradius, float yaw)
{
    gle::colorf(0.5f, 0.5f, 0.5f);
    gle::defvertex();
    gle::begin(GL_LINE_LOOP);
    loopi(15)
    {
        const vec2 &sc = sincos360[i*(360/15)];
        gle::attrib(vec(xradius*sc.x, yradius*sc.y, 0).rotate_around_z((yaw+90)*RAD).add(o));
    }
    xtraverts += gle::end();
}

struct batchedmodel
{
    vec pos, color, dir;
    const bvec *material;
    int anim;
    float yaw, pitch, roll, transparent, sizescale;
    int basetime, basetime2, flags;
    dynent *d;
    int attached;
    occludequery *query;
};
struct modelbatch
{
    model *m;
    int flags;
    vector<batchedmodel> batched;
};
static vector<modelbatch *> batches;
static vector<modelattach> modelattached;
static int numbatches = -1;
static occludequery *modelquery = NULL;

void startmodelbatches()
{
    numbatches = 0;
    modelattached.setsize(0);
}

modelbatch &addbatchedmodel(model *m)
{
    modelbatch *b = NULL;
    if(m->batch>=0 && m->batch<numbatches && batches[m->batch]->m==m) b = batches[m->batch];
    else
    {
        if(numbatches<batches.length())
        {
            b = batches[numbatches];
            b->batched.setsize(0);
        }
        else b = batches.add(new modelbatch);
        b->m = m;
        b->flags = 0;
        m->batch = numbatches++;
    }
    return *b;
}

void renderbatchedmodel(model *m, batchedmodel &b)
{
    modelattach *a = NULL;
    if(b.attached>=0) a = &modelattached[b.attached];

    int anim = b.anim;
    if(shadowmapping)
    {
        anim |= ANIM_NOSKIN;
        GLOBALPARAMF(shadowintensity, b.transparent);
    }
    else
    {
        if(b.flags&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

    m->render(anim, b.basetime, b.basetime2, b.pos, b.yaw, b.pitch, b.roll, b.d, a, b.color, b.dir, b.material, b.transparent, b.sizescale);
}

struct transparentmodel
{
    model *m;
    batchedmodel *batched;
    float dist;
};

static inline bool sorttransparentmodels(const transparentmodel &x, const transparentmodel &y)
{
    return x.dist < y.dist;
}

void endmodelbatches()
{
    vector<transparentmodel> transparent;
    loopi(numbatches)
    {
        modelbatch &b = *batches[i];
        if(b.batched.empty()) continue;
        if(b.flags&(MDL_SHADOW|MDL_DYNSHADOW))
        {
            vec center, bbradius;
            b.m->boundbox(center, bbradius);
            loopvj(b.batched)
            {
                batchedmodel &bm = b.batched[j];
                if(bm.flags&(MDL_SHADOW|MDL_DYNSHADOW))
                    renderblob(bm.flags&MDL_DYNSHADOW ? BLOB_DYNAMIC : BLOB_STATIC, bm.d && bm.d->ragdoll ? bm.d->ragdoll->center : bm.pos, (bm.d ? bm.d->radius : max(bbradius.x, bbradius.y))*bm.sizescale, bm.transparent);
            }
            flushblobs();
        }
        bool rendered = false;
        occludequery *query = NULL;
        loopvj(b.batched)
        {
            batchedmodel &bm = b.batched[j];
            if(bm.flags&MDL_CULL_VFC) continue;
            if(bm.query!=query)
            {
                if(query) endquery(query);
                query = bm.query;
                if(query) startquery(query);
            }
            if(bm.transparent < 1 && (!query || query->owner==bm.d) && !shadowmapping)
            {
                transparentmodel &tm = transparent.add();
                tm.m = b.m;
                tm.batched = &bm;
                tm.dist = camera1->o.dist(bm.d && bm.d->ragdoll ? bm.d->ragdoll->center : bm.pos);
                continue;
            }
            if(!rendered) { b.m->startrender(); rendered = true; }
            renderbatchedmodel(b.m, bm);
        }
        if(query) endquery(query);
        if(rendered) b.m->endrender();
    }
    if(transparent.length())
    {
        transparent.sort(sorttransparentmodels);
        model *lastmodel = NULL;
        occludequery *query = NULL;
        loopv(transparent)
        {
            transparentmodel &tm = transparent[i];
            if(lastmodel!=tm.m)
            {
                if(lastmodel) lastmodel->endrender();
                (lastmodel = tm.m)->startrender();
            }
            if(query!=tm.batched->query)
            {
                if(query) endquery(query);
                query = tm.batched->query;
                if(query) startquery(query);
            }
            renderbatchedmodel(tm.m, *tm.batched);
        }
        if(query) endquery(query);
        if(lastmodel) lastmodel->endrender();
    }
    numbatches = -1;
}

void startmodelquery(occludequery *query)
{
    modelquery = query;
}

void endmodelquery()
{
    int querybatches = 0;
    loopi(numbatches)
    {
        modelbatch &b = *batches[i];
        if(b.batched.empty() || b.batched.last().query!=modelquery) continue;
        querybatches++;
    }
    if(querybatches<=1)
    {
        if(!querybatches) modelquery->fragments = 0;
        modelquery = NULL;
        return;
    }
    flushblobs();
    int minattached = modelattached.length();
    startquery(modelquery);
    loopi(numbatches)
    {
        modelbatch &b = *batches[i];
        if(b.batched.empty() || b.batched.last().query!=modelquery) continue;
        b.m->startrender();
        do
        {
            batchedmodel &bm = b.batched.pop();
            if(bm.attached>=0) minattached = min(minattached, bm.attached);
            renderbatchedmodel(b.m, bm);
        }
        while(b.batched.length() && b.batched.last().query==modelquery);
        b.m->endrender();
    }
    endquery(modelquery);
    modelquery = NULL;
    modelattached.setsize(minattached);
}

VAR(0, maxmodelradiusdistance, 10, 200, 1000);

static inline void enablecullmodelquery()
{
    startbb();
}

static inline void rendercullmodelquery(model *m, dynent *d, const vec &center, float radius)
{
    if(std::fabs(camera1->o.x-center.x) < radius+1 &&
       std::fabs(camera1->o.y-center.y) < radius+1 &&
       std::fabs(camera1->o.z-center.z) < radius+1)
    {
        d->query = NULL;
        return;
    }
    d->query = newquery(d);
    if(!d->query) return;
    startquery(d->query);
    int br = int(radius*2)+1;
    drawbb(ivec(int(center.x-radius), int(center.y-radius), int(center.z-radius)), ivec(br, br, br));
    endquery(d->query);
}

static inline void disablecullmodelquery()
{
    endbb();
}

static inline int cullmodel(model *m, const vec &center, float radius, int flags, dynent *d = NULL, bool shadow = false)
{
    if(flags&MDL_CULL_DIST && center.dist(camera1->o)/radius>maxmodelradiusdistance) return MDL_CULL_DIST;
    if(flags&MDL_CULL_VFC)
    {
        if(reflecting || refracting)
        {
            if(reflecting || refracting>0)
            {
                if(center.z+radius<=reflectz) return MDL_CULL_VFC;
            }
            else
            {
                if(fogging && center.z+radius<reflectz-refractfog) return MDL_CULL_VFC;
                if(!shadow && center.z-radius>=reflectz) return MDL_CULL_VFC;
            }
            if(center.dist(camera1->o)-radius>reflectdist) return MDL_CULL_VFC;
        }
        if(isfoggedsphere(radius, center)) return MDL_CULL_VFC;
        if(shadowmapping && !isshadowmapcaster(center, radius)) return MDL_CULL_VFC;
    }
    if(shadowmapping)
    {
        if(d)
        {
            if(flags&MDL_CULL_OCCLUDED && d->occluded>=OCCLUDE_PARENT) return MDL_CULL_OCCLUDED;
            if(flags&MDL_CULL_QUERY && d->occluded+1>=OCCLUDE_BB && d->query && d->query->owner==d && checkquery(d->query)) return MDL_CULL_QUERY;
        }
        if(!addshadowmapcaster(center, radius, radius)) return MDL_CULL_VFC;
    }
    else if(flags&MDL_CULL_OCCLUDED && modeloccluded(center, radius))
    {
        if(!reflecting && !refracting && d) d->occluded = OCCLUDE_PARENT;
        return MDL_CULL_OCCLUDED;
    }
    else if(flags&MDL_CULL_QUERY && d->query && d->query->owner==d && checkquery(d->query))
    {
        if(!reflecting && !refracting && d->occluded<OCCLUDE_BB) d->occluded++;
        return MDL_CULL_QUERY;
    }
    return 0;
}

void renderradius(const vec &o, float radius)
{
    gle::colorf(0.5f, 0.5f, 0.5f);
    gle::defvertex();
    loopk(3)
    {
        gle::begin(GL_LINE_LOOP);
        loopi(16)
        {
            vec d = o;
            switch(k)
            {
                case 0:
                    d.add(vec(radius*cosf(2*M_PI*i/16.0f), radius*sinf(2*M_PI*i/16.0f), 0).rotate_around_x(90*RAD));
                    break;
                case 1:
                    d.add(vec(radius*cosf(2*M_PI*i/16.0f), radius*sinf(2*M_PI*i/16.0f), 0).rotate_around_y(-90*RAD));
                    break;
                case 2: default:
                    d.add(vec(radius*cosf(2*M_PI*i/16.0f), radius*sinf(2*M_PI*i/16.0f), 0).rotate_around_z(90*RAD));
                    break;
            }
            gle::attrib(d);
        }
        xtraverts += gle::end();
    }
}

void rendermodel(entitylight *light, const char *mdl, int anim, const vec &o, float yaw, float pitch, float roll, int flags, dynent *d, modelattach *a, int basetime, int basetime2, float trans, float size)
{
    if((shadowmapping && !(flags&(MDL_SHADOW|MDL_DYNSHADOW))) || trans <= 0 || size <= 0) return;
    model *m = loadmodel(mdl);
    if(!m) return;
    vec center(0, 0, 0), bbradius(0, 0, 0);
    float radius = 0;
    bool shadow = !shadowmap && !glaring && (flags&(MDL_SHADOW|MDL_DYNSHADOW)) && showblobs;

    if(flags&(MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_CULL_QUERY|MDL_SHADOW|MDL_DYNSHADOW))
    {
        if(flags&MDL_CULL_QUERY)
        {
            if(!oqfrags || !oqdynent || !d) flags &= ~MDL_CULL_QUERY;
        }

        m->boundbox(center, bbradius);
        radius = bbradius.magnitude();
        if(d && d->ragdoll)
        {
            radius = max(radius, d->ragdoll->radius);
            center = d->ragdoll->center;
        }
        else
        {
            center.mul(size);
            center.rotate_around_x(-roll*RAD);
            center.rotate_around_z(yaw*RAD);
            center.add(o);
        }
        radius *= size;

        int culled = cullmodel(m, center, radius, flags, d, shadow);
        if(culled)
        {
            if(culled&(MDL_CULL_OCCLUDED|MDL_CULL_QUERY) && flags&MDL_CULL_QUERY && !reflecting && !refracting)
            {
                enablecullmodelquery();
                rendercullmodelquery(m, d, center, radius);
                disablecullmodelquery();
            }
            return;
        }

        if(reflecting || refracting || shadowmapping) flags &= ~MDL_CULL_QUERY;
    }

    if(flags&MDL_NORENDER) anim |= ANIM_NORENDER;
    else if(showboundingbox && !shadowmapping && !reflecting && !refracting)
    {
        notextureshader->set();
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        if(d && showboundingbox==1 && (d->type == ENT_PLAYER || d->type == ENT_AI))
        {
            if(d->ragdoll && (d->state == CS_DEAD || d->state == CS_WAITING))
            {
                loopv(d->ragdoll->skel->verts) renderradius(d->ragdoll->verts[i].pos, d->ragdoll->skel->verts[i].radius);
                gle::colorf(0.5f, 1, 1);
                gle::defvertex();
                loopv(d->ragdoll->skel->tris)
                {
                    ragdollskel::tri &t = d->ragdoll->skel->tris[i];
                    gle::begin(GL_LINE_LOOP);
                    loopk(3) gle::attrib(d->ragdoll->verts[t.vert[k]].pos);
                    gle::end();
                }
            }
            else physics::complexboundbox(d);
        }
        else
        {
            vec center, radius;
            if(showboundingbox==1) m->collisionbox(center, radius);
            else m->boundbox(center, radius);
            center.mul(size);
            radius.mul(size);
            matrix4x3 m;
            m.identity();
            m.settranslation(o);
            m.rotate_around_z(yaw*RAD);
            m.rotate_around_x(roll*-RAD);
            m.rotate_around_y(pitch*-RAD);
            render3dbox(center, radius.z, radius.z, radius.x, radius.y, &m);
        }

        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);
    }

    vec lightcolor(1, 1, 1), lightdir(0, 0, 1);
    const bvec *lightmaterial = NULL;
    if(!shadowmapping)
    {
        vec pos = o;
        if(d)
        {
            if(!reflecting && !refracting) d->occluded = OCCLUDE_NOTHING;
            if(!light) light = &d->light;
            if(flags&MDL_LIGHT && light->millis!=lastmillis)
            {
                if(d->ragdoll)
                {
                    pos = d->ragdoll->center;
                    pos.z += radius/2;
                }
                else if(d->type < ENT_CAMERA) pos.z += 0.75f*(d->height + d->aboveeye);
                lightreaching(pos, light->color, light->dir, (flags&MDL_LIGHT_FAST)!=0);
                dynlightreaching(pos, light->color, light->dir);
                light->millis = lastmillis;
            }
        }
        else if(flags&MDL_LIGHT)
        {
            if(!light)
            {
                lightreaching(pos, lightcolor, lightdir, (flags&MDL_LIGHT_FAST)!=0);
                dynlightreaching(pos, lightcolor, lightdir);
            }
            else if(light->millis!=lastmillis)
            {
                lightreaching(pos, light->color, light->dir, (flags&MDL_LIGHT_FAST)!=0);
                dynlightreaching(pos, light->color, light->dir);
                light->millis = lastmillis;
            }
        }
        if(light)
        {
            lightcolor = light->color;
            lightdir = light->dir;
            lightmaterial = light->material;
            if(flags&MDL_LIGHTFX) lightcolor.max(light->effect).lerp(light->effect, 0.6f*max(max(light->effect.x, light->effect.y), light->effect.z));
        }
        if(flags&MDL_DYNLIGHT) dynlightreaching(pos, lightcolor, lightdir);
    }

    if(a) for(int i = 0; a[i].tag; i++)
    {
        if(a[i].name) a[i].m = loadmodel(a[i].name);
        //if(a[i].m && a[i].m->type()!=m->type()) a[i].m = NULL;
    }

    if(numbatches>=0)
    {
        modelbatch &mb = addbatchedmodel(m);
        batchedmodel &b = mb.batched.add();
        b.query = modelquery;
        b.pos = o;
        b.color = lightcolor;
        b.dir = lightdir;
        b.material = lightmaterial;
        b.anim = anim;
        b.yaw = yaw;
        b.pitch = pitch;
        b.roll = roll;
        b.basetime = basetime;
        b.basetime2 = basetime2;
        b.transparent = trans;
        b.sizescale = size;
        b.flags = flags & ~(MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED);
        if(!shadow || reflecting || refracting>0)
        {
            b.flags &= ~(MDL_SHADOW|MDL_DYNSHADOW);
            if((flags&MDL_CULL_VFC) && refracting<0 && center.z-radius>=reflectz) b.flags |= MDL_CULL_VFC;
        }
        mb.flags |= b.flags;
        b.d = d;
        b.attached = a ? modelattached.length() : -1;
        if(a) for(int i = 0;; i++) { modelattached.add(a[i]); if(!a[i].tag) break; }
        if(flags&MDL_CULL_QUERY) d->query = b.query = newquery(d);
        return;
    }

    if(shadow && !reflecting && refracting<=0)
    {
        renderblob(flags&MDL_DYNSHADOW ? BLOB_DYNAMIC : BLOB_STATIC, d && d->ragdoll ? center : o, (d ? d->radius : max(bbradius.x, bbradius.y)) * size, trans);
        flushblobs();
        if((flags&MDL_CULL_VFC) && refracting<0 && center.z-radius>=reflectz) return;
    }

    m->startrender();

    if(shadowmapping)
    {
        anim |= ANIM_NOSKIN;
        GLOBALPARAMF(shadowintensity, trans);
    }
    else
    {
        if(flags&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

    if(flags&MDL_CULL_QUERY)
    {
        d->query = newquery(d);
        if(d->query) startquery(d->query);
    }

    m->render(anim, basetime, basetime2, o, yaw, pitch, roll, d, a, lightcolor, lightdir, lightmaterial, trans, size);

    if(flags&MDL_CULL_QUERY && d->query) endquery(d->query);

    m->endrender();
}

void abovemodel(vec &o, const char *mdl)
{
    model *m = loadmodel(mdl);
    if(!m) return;
    o.z += m->above();
}

bool matchanim(const char *name, const char *pattern)
{
    for(;; pattern++)
    {
        const char *s = name;
        char c;
        for(;; pattern++)
        {
            c = *pattern;
            if(!c || c=='|') break;
            else if(c=='*')
            {
                if(!*s || iscubespace(*s)) break;
                do s++; while(*s && !iscubespace(*s));
            }
            else if(c!=*s) break;
            else s++;
        }
        if(!*s && (!c || c=='|')) return true;
        pattern = strchr(pattern, '|');
        if(!pattern) break;
    }
    return false;
}

ICOMMAND(0, findanims, "s", (char *name),
{
    vector<int> anims;
    game::findanims(name, anims);
    vector<char> buf;
    string num;
    loopv(anims)
    {
        formatstring(num, "%d", anims[i]);
        if(i > 0) buf.add(' ');
        buf.put(num, strlen(num));
    }
    buf.add('\0');
    result(buf.getbuf());
});

void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks) // model skin sharing
{
    string dirs[3];
    formatstring(dirs[0], "%s/", dir);
    formatstring(dirs[1], "%s/", altdir);
    formatstring(dirs[2], "textures/");
    masks = notexture;

    #define tryload(tex, prefix, cmd, path) loopi(4) { if((tex = textureload(makerelpath(i < 3 ? dirs[i] : "", path, prefix, cmd), 0, true, false)) != notexture) break; }
    tryload(skin, NULL, NULL, "skin");
    tryload(masks, NULL, NULL, "masks");
}

void setbbfrommodel(dynent *d, const char *mdl, float size)
{
    model *m = loadmodel(mdl);
    if(!m) return;
    vec center, radius;
    m->collisionbox(center, radius);
    if(d->type==ENT_INANIMATE && !m->ellipsecollide)
        d->collidetype = COLLIDE_OBB;
    d->xradius  = (radius.x + std::fabs(center.x))*size;
    d->yradius  = (radius.y + std::fabs(center.y))*size;
    d->radius   = d->collidetype==COLLIDE_OBB ? sqrtf(d->xradius*d->xradius + d->yradius*d->yradius) : max(d->xradius, d->yradius);
    d->height   = (d->zradius = (center.z-radius.z) + radius.z*2*m->height)*size;
    d->aboveeye = radius.z*2*(1.0f-m->height);
}

