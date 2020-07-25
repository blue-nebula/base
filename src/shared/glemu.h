#ifndef GLEMU_H
#define GLEMU_H

namespace gle
{
    enum
    {
        ATTRIB_VERTEX = 0,
        ATTRIB_COLOR = 1,
        ATTRIB_TEXCOORD0 = 2,
        ATTRIB_TEXCOORD1 = 3,
        ATTRIB_NORMAL = 4,
        ATTRIB_TANGENT = 5,
        ATTRIB_BONEWEIGHT = 6,
        ATTRIB_BONEINDEX = 7,
        MAXATTRIBS = 8
    };

    extern const char * const attribnames[MAXATTRIBS];
    extern ucharbuf attribbuf;

    extern int enabled;
    extern void forcedisable();
    static inline void disable() { if(enabled) forcedisable(); }

    extern void begin(GLenum mode);
    extern void begin(GLenum mode, int numverts);
    extern void multidraw();
    extern void defattrib(int type, int size, int format);

    static inline void defvertex(int size = 3, int format = GL_FLOAT) { defattrib(ATTRIB_VERTEX, size, format); }
    static inline void defcolor(int size = 3, int format = GL_FLOAT) { defattrib(ATTRIB_COLOR, size, format); }
    static inline void deftexcoord0(int size = 2, int format = GL_FLOAT) { defattrib(ATTRIB_TEXCOORD0, size, format); }
    static inline void defnormal(int size = 3, int format = GL_FLOAT) { defattrib(ATTRIB_NORMAL, size, format); }
    static inline void deftangent(int size = 4, int format = GL_FLOAT) { defattrib(ATTRIB_TANGENT, size, format); }

    static inline void colorf(float x, float y, float z) { glVertexAttrib3f_(ATTRIB_COLOR, x, y, z); }
    static inline void colorf(float x, float y, float z, float w) { glVertexAttrib4f_(ATTRIB_COLOR, x, y, z, w); }
    static inline void color(const vec &v) { glVertexAttrib3fv_(ATTRIB_COLOR, v.v); }
    static inline void color(const vec &v, float w) { glVertexAttrib4f_(ATTRIB_COLOR, v.x, v.y, v.z, w); }
    static inline void colorub(uchar x, uchar y, uchar z, uchar w = 255) { glVertexAttrib4Nub_(ATTRIB_COLOR, x, y, z, w); }
    static inline void color(const bvec &v, uchar alpha = 255) { glVertexAttrib4Nub_(ATTRIB_COLOR, v.x, v.y, v.z, alpha); }
    static inline void color(const bvec4 &v) { glVertexAttrib4Nubv_(ATTRIB_COLOR, v.v); }

    static inline void enablevertex() { disable(); glEnableVertexAttribArray_(ATTRIB_VERTEX); }
    static inline void disablevertex() { glDisableVertexAttribArray_(ATTRIB_VERTEX); }
    static inline void vertexpointer(int stride, const void *data, GLenum type = GL_FLOAT, int size = 3, GLenum normalized = GL_FALSE) { disable(); glVertexAttribPointer_(ATTRIB_VERTEX, size, type, normalized, stride, data); }

    static inline void enablecolor() { ; glEnableVertexAttribArray_(ATTRIB_COLOR); }
    static inline void disablecolor() { glDisableVertexAttribArray_(ATTRIB_COLOR); }
    static inline void colorpointer(int stride, const void *data, GLenum type = GL_UNSIGNED_BYTE, int size = 4, GLenum normalized = GL_TRUE) { ; glVertexAttribPointer_(ATTRIB_COLOR, size, type, normalized, stride, data); }

    static inline void enabletexcoord0() { ; glEnableVertexAttribArray_(ATTRIB_TEXCOORD0); }
    static inline void disabletexcoord0() { glDisableVertexAttribArray_(ATTRIB_TEXCOORD0); }
    static inline void texcoord0pointer(int stride, const void *data, GLenum type = GL_FLOAT, int size = 2, GLenum normalized = GL_FALSE) { ; glVertexAttribPointer_(ATTRIB_TEXCOORD0, size, type, normalized, stride, data); }

    static inline void enabletexcoord1() { ; glEnableVertexAttribArray_(ATTRIB_TEXCOORD1); }
    static inline void disabletexcoord1() { glDisableVertexAttribArray_(ATTRIB_TEXCOORD1); }
    static inline void texcoord1pointer(int stride, const void *data, GLenum type = GL_FLOAT, int size = 2, GLenum normalized = GL_FALSE) { ; glVertexAttribPointer_(ATTRIB_TEXCOORD1, size, type, normalized, stride, data); }

    static inline void enablenormal() { ; glEnableVertexAttribArray_(ATTRIB_NORMAL); }
    static inline void disablenormal() { glDisableVertexAttribArray_(ATTRIB_NORMAL); }
    static inline void normalpointer(int stride, const void *data, GLenum type = GL_FLOAT, int size = 3, GLenum normalized = GL_TRUE) { ; glVertexAttribPointer_(ATTRIB_NORMAL, size, type, normalized, stride, data); }

    static inline void enabletangent() { ; glEnableVertexAttribArray_(ATTRIB_TANGENT); }
    static inline void disabletangent() { glDisableVertexAttribArray_(ATTRIB_TANGENT); }
    static inline void tangentpointer(int stride, const void *data, GLenum type = GL_FLOAT, int size = 4, GLenum normalized = GL_TRUE) { ; glVertexAttribPointer_(ATTRIB_TANGENT, size, type, normalized, stride, data); }

    static inline void enableboneweight() { ; glEnableVertexAttribArray_(ATTRIB_BONEWEIGHT); }
    static inline void disableboneweight() { glDisableVertexAttribArray_(ATTRIB_BONEWEIGHT); }
    static inline void boneweightpointer(int stride, const void *data, GLenum type = GL_UNSIGNED_BYTE, int size = 4, GLenum normalized = GL_TRUE) { ; glVertexAttribPointer_(ATTRIB_BONEWEIGHT, size, type, normalized, stride, data); }

    static inline void enableboneindex() { ; glEnableVertexAttribArray_(ATTRIB_BONEINDEX); }
    static inline void disableboneindex() { glDisableVertexAttribArray_(ATTRIB_BONEINDEX); }
    static inline void boneindexpointer(int stride, const void *data, GLenum type = GL_UNSIGNED_BYTE, int size = 4, GLenum normalized = GL_FALSE) { ; glVertexAttribPointer_(ATTRIB_BONEINDEX, size, type, normalized, stride, data); }

    static inline void bindebo(GLuint ebo) { disable(); glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, ebo); }
    static inline void clearebo() { glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER, 0); }
    static inline void bindvbo(GLuint vbo) { disable(); glBindBuffer_(GL_ARRAY_BUFFER, vbo); }
    static inline void clearvbo() { glBindBuffer_(GL_ARRAY_BUFFER, 0); }

    template<class T>
    static inline void attrib(T x)
    {
        if(attribbuf.check(sizeof(T)))
        {
            T *buf = (T *)attribbuf.pad(sizeof(T));
            buf[0] = x;
        }
    }

    template<class T>
    static inline void attrib(T x, T y)
    {
        if(attribbuf.check(2*sizeof(T)))
        {
            T *buf = (T *)attribbuf.pad(2*sizeof(T));
            buf[0] = x;
            buf[1] = y;
        }
    }

    template<class T>
    static inline void attrib(T x, T y, T z)
    {
        if(attribbuf.check(3*sizeof(T)))
        {
            T *buf = (T *)attribbuf.pad(3*sizeof(T));
            buf[0] = x;
            buf[1] = y;
            buf[2] = z;
        }
    }

    template<class T>
    static inline void attrib(T x, T y, T z, T w)
    {
        if(attribbuf.check(4*sizeof(T)))
        {
            T *buf = (T *)attribbuf.pad(4*sizeof(T));
            buf[0] = x;
            buf[1] = y;
            buf[2] = z;
            buf[3] = w;
        }
    }

    static inline void attribf(float x, float y) { attrib<float>(x, y); }
    static inline void attribf(float x, float y, float z) { attrib<float>(x, y, z); }
    static inline void attribf(float x, float y, float z, float w) { attrib<float>(x, y, z, w); }
    static inline void attribs(short x, short y) { attrib<short>(x, y); }
    static inline void attribs(short x, short y, short z) { attrib<short>(x, y, z); }
    static inline void attribs(short x, short y, short z, short w) { attrib<short>(x, y, z, w); }
    static inline void attribus(ushort x, ushort y) { attrib<ushort>(x, y); }
    static inline void attribus(ushort x, ushort y, ushort z) { attrib<ushort>(x, y, z); }
    static inline void attribus(ushort x, ushort y, ushort z, ushort w) { attrib<ushort>(x, y, z, w); }
    static inline void attribi(int x, int y) { attrib<int>(x, y); }
    static inline void attribi(int x, int y, int z) { attrib<int>(x, y, z); }
    static inline void attribi(int x, int y, int z, int w) { attrib<int>(x, y, z, w); }
    static inline void attribui(uint x, uint y) { attrib<uint>(x, y); }
    static inline void attribui(uint x, uint y, uint z) { attrib<uint>(x, y, z); }
    static inline void attribui(uint x, uint y, uint z, uint w) { attrib<uint>(x, y, z, w); }

    static inline void attrib(const vec &v, float w) { attribf(v.x, v.y, v.z, w); }

    extern int end();

    extern void enablequads();
    extern void disablequads();
    extern void drawquads(int offset, int count);

    extern void setup();
    extern void cleanup();
}

#endif
