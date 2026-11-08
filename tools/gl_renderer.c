#define _POSIX_C_SOURCE 200809L
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glcorearb.h>
#include "gl_renderer.h"
#include "gpu_plan.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {DrawImage image;GLuint texture,u,v;} Cached;
struct GlRenderer {
    EGLDisplay display;EGLContext context;EGLSurface surface;
    DrawList list;GpuPlan plan;
    GLuint ui,swirl,background,copy,cap,video,vao,vbo,full,uniform;
    GLuint frame,ambient[BLADE_COUNT],ambient_fbo[BLADE_COUNT],base;
    Cached cache[512];unsigned cached;
    unsigned source_frame[BLADE_COUNT];bool source_valid[BLADE_COUNT];
    unsigned char *pixels;
};
static GLuint shader(const char *name,GLenum kind) {
    char path[1024];snprintf(path,sizeof path,"%s/%s",BLADES_SHADER_DIR,name);
    FILE *f=fopen(path,"rb");if(!f)return 0;
    char input[16384];size_t n=fread(input,1,sizeof input-1,f);fclose(f);input[n]=0;
    char text[18000]="#version 450 core\n#define BLADES_OPENGL 1\n";
    char *line=strchr(input,'\n');if(!line)return 0;line++;
    while(*line) {
        char *end=strchr(line,'\n');size_t bytes=end?(size_t)(end-line):strlen(line);
        if(!strncmp(line,"layout(set=0,binding=",21)){char layout[64];snprintf(layout,sizeof layout,"layout(binding=%u)",(unsigned)(line[21]-'0'));strcat(text,layout);}
        else if(!strncmp(line,"layout(push_constant)",21))strcat(text,"layout(std140,binding=0)");
        else {strncat(text,line,bytes);strcat(text,"\n");if(!end)break;line=end+1;continue;}
        size_t prefix=!strncmp(line,"layout(set=0,binding=",21)?23:21;
        strncat(text,line+prefix,bytes-prefix);strcat(text,"\n");if(!end)break;line=end+1;
    }
    GLuint s=glCreateShader(kind);const char *source=text;glShaderSource(s,1,&source,NULL);glCompileShader(s);
    GLint ok;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[4096];glGetShaderInfoLog(s,sizeof log,NULL,log);fprintf(stderr,"%s: %s\n",name,log);glDeleteShader(s);return 0;}
    return s;
}
static GLuint program(const char *fragment) {
    GLuint v=shader("ui.vert",GL_VERTEX_SHADER),f=shader(fragment,GL_FRAGMENT_SHADER);
    if(!v||!f)return 0;
    GLuint p=glCreateProgram();glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);glDeleteShader(v);glDeleteShader(f);
    GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){char log[4096];glGetProgramInfoLog(p,sizeof log,NULL,log);fprintf(stderr,"link %s: %s\n",fragment,log);glDeleteProgram(p);return 0;}
    return p;
}
static GLuint texture(int w,int h,const void *data,bool mask) {
    GLuint t;glGenTextures(1,&t);glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,mask?GL_R8:GL_RGBA8,w,h,0,mask?GL_RED:GL_RGBA,GL_UNSIGNED_BYTE,data);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    if(mask)glTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_RGBA,(GLint[]){GL_ONE,GL_ONE,GL_ONE,GL_RED});
    return t;
}
static void target(GLuint framebuffer,int w,int h) {glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);glViewport(0,0,w,h);glDisable(GL_BLEND);}
static bool attach(GLuint *fbo,GLuint tex) {
    glGenFramebuffers(1,fbo);glBindFramebuffer(GL_FRAMEBUFFER,*fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0);
    return glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
}
static void bind_vertices(GLuint buffer) {
    glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(GpuVertex),(void *)0);
    glVertexAttribPointer(1,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(GpuVertex),(void *)16);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(GpuVertex),(void *)8);
}
static void constants(GlRenderer *r,const float *data,size_t bytes) {
    glBindBuffer(GL_UNIFORM_BUFFER,r->uniform);glBufferSubData(GL_UNIFORM_BUFFER,0,(GLsizeiptr)bytes,data);
    glBindBufferBase(GL_UNIFORM_BUFFER,0,r->uniform);
}
static bool same(DrawImage a,DrawImage b) {return a.id==b.id&&a.pixels==b.pixels&&a.width==b.width&&a.height==b.height&&a.format==b.format;}
static Cached *find(GlRenderer *r,DrawImage image) {
    for(unsigned i=0;i<r->cached;i++)if(same(r->cache[i].image,image))return r->cache+i;
    return NULL;
}
static void plane_upload(GLuint t,int width,int height,const void *data,int pitch){
    glBindTexture(GL_TEXTURE_2D,t);glPixelStorei(GL_UNPACK_ROW_LENGTH,pitch);
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height,GL_RED,GL_UNSIGNED_BYTE,data);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
}
static GLuint plane(int w,int h){
    GLuint t=texture(w,h,NULL,true);
    glTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_SWIZZLE_RGBA,(GLint[]){GL_RED,GL_ZERO,GL_ZERO,GL_ONE});
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);return t;
}
static bool upload(GlRenderer *r,DrawImage image) {
    Cached *c=find(r,image);bool yuv=image_yuv(image.format);bool fresh=!c;
    if(!c) {
        if(r->cached==512)return false;
        c=r->cache+r->cached++;
        if(yuv){c->texture=plane(image.width,image.height);c->u=plane(image.width/2,image.height/2);c->v=plane(image.width/2,image.height/2);}
        else c->texture=texture(image.width,image.height,image.pixels,image.format==IMAGE_MASK);
    }
    if(yuv&&(fresh||c->image.revision!=image.revision)){
        const unsigned char *uv=image.pixels+(size_t)image.width*image.height;
        plane_upload(c->texture,image.width,image.height,image.pixels,image.width);
        plane_upload(c->u,image.width/2,image.height/2,uv,image.width);
        plane_upload(c->v,image.width/2,image.height/2,uv+image.width/2,image.width);
    } else if(!fresh&&c->image.revision!=image.revision) {
        glBindTexture(GL_TEXTURE_2D,c->texture);glTexSubImage2D(GL_TEXTURE_2D,0,0,0,image.width,image.height,
            image.format==IMAGE_MASK?GL_RED:GL_RGBA,GL_UNSIGNED_BYTE,image.pixels);
    }
    c->image=image;return true;
}
GlRenderer *gl_renderer_open(void) {
    GlRenderer *r=calloc(1,sizeof *r);if(!r)return NULL;
    setenv("EGL_PLATFORM","surfaceless",0);
    r->display=eglGetDisplay(EGL_DEFAULT_DISPLAY);EGLint major,minor;
    if(!eglInitialize(r->display,&major,&minor)||!eglBindAPI(EGL_OPENGL_API))goto fail;
    EGLConfig config;EGLint count;
    EGLint attrs[]={EGL_SURFACE_TYPE,EGL_PBUFFER_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_BIT,EGL_RED_SIZE,8,
        EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_NONE};
    if(!eglChooseConfig(r->display,attrs,&config,1,&count)||!count)goto fail;
    EGLint context[]={EGL_CONTEXT_MAJOR_VERSION,4,EGL_CONTEXT_MINOR_VERSION,5,EGL_CONTEXT_OPENGL_PROFILE_MASK,EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,EGL_NONE};
    r->context=eglCreateContext(r->display,config,EGL_NO_CONTEXT,context);
    r->surface=eglCreatePbufferSurface(r->display,config,(EGLint[]){EGL_WIDTH,WIDTH,EGL_HEIGHT,HEIGHT,EGL_NONE});
    if(r->context==EGL_NO_CONTEXT||r->surface==EGL_NO_SURFACE||!eglMakeCurrent(r->display,r->surface,r->surface,r->context))goto fail;
    fprintf(stderr,"GPU validation backend: %s / %s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));
    r->ui=program("ui.frag");r->swirl=program("swirl.frag");r->background=program("background.frag");r->copy=program("copy.frag");r->cap=program("cap.frag");r->video=program("video.frag");
    if(!r->ui||!r->swirl||!r->background||!r->copy||!r->cap||!r->video)goto fail;
    glGenVertexArrays(1,&r->vao);glBindVertexArray(r->vao);
    glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glEnableVertexAttribArray(2);
    glGenBuffers(1,&r->vbo);glGenBuffers(1,&r->full);glGenBuffers(1,&r->uniform);
    glBindBuffer(GL_UNIFORM_BUFFER,r->uniform);glBufferData(GL_UNIFORM_BUFFER,64,NULL,GL_DYNAMIC_DRAW);
    const GpuVertex quad[]={
        {-1,1,0,0,0xffffffff},{1,1,1,0,0xffffffff},{1,-1,1,1,0xffffffff},
        {-1,1,0,0,0xffffffff},{1,-1,1,1,0xffffffff},{-1,-1,0,1,0xffffffff}};
    glBindBuffer(GL_ARRAY_BUFFER,r->full);glBufferData(GL_ARRAY_BUFFER,sizeof quad,quad,GL_STATIC_DRAW);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);glDisable(GL_DITHER);glDisable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);
    glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    GLuint frame=texture(WIDTH,HEIGHT,NULL,false);if(!attach(&r->frame,frame))goto fail;
    /* The frame texture is owned by its framebuffer attachment until close. */
    r->base=texture(WIDTH,HEIGHT,NULL,false);
    r->pixels=malloc(WIDTH*HEIGHT*4);if(!r->pixels)goto fail;
    return r;
fail:
    fprintf(stderr,"GPU validation setup failed: EGL=%04x GL=%04x\n",eglGetError(),r->context?glGetError():0);
    gl_renderer_close(r);return NULL;
}
bool gl_renderer_frame(GlRenderer *r,const Dashboard *d,Canvas *readback) {
    if(draw_list_update(&r->list,d)) {
        if(!gpu_plan_build(&r->plan,&r->list))return false;
        for(size_t i=0;i<r->plan.image_count;i++)if(!upload(r,r->plan.images[i]))return false;
        glBindBuffer(GL_ARRAY_BUFFER,r->vbo);glBufferData(GL_ARRAY_BUFFER,
            (GLsizeiptr)(r->plan.vertex_count*sizeof(GpuVertex)),r->plan.vertices,GL_DYNAMIC_DRAW);
    }
    unsigned frame=gpu_ambient_frame(d->elapsed);
    for(int b=0;b<BLADE_COUNT;b++)if((r->plan.ambient_blades&(1u<<b))&&(!r->source_valid[b]||r->source_frame[b]!=frame)) {
        if(!r->ambient[b]){r->ambient[b]=texture(384,216,NULL,false);if(!attach(r->ambient_fbo+b,r->ambient[b]))return false;}
        target(r->ambient_fbo[b],384,216);glUseProgram(r->swirl);
        float values[8];gpu_ambient_constants(b,frame,values);constants(r,values,sizeof values);
        bind_vertices(r->full);glDrawArrays(GL_TRIANGLES,0,6);r->source_valid[b]=true;r->source_frame[b]=frame;
    }
    target(r->frame,WIDTH,HEIGHT);
    for(size_t i=0;i<r->plan.batch_count;i++) {
        GpuBatch *batch=r->plan.batches+i;
        if(batch->kind==DRAW_FADE_BEGIN) {
            glBindTexture(GL_TEXTURE_2D,r->base);glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,WIDTH,HEIGHT);
        } else if(batch->kind==DRAW_FADE_END) {
            glUseProgram(r->copy);float values[]={1-(float)(unsigned)(batch->amount*256)/256,0,0,0};constants(r,values,sizeof values);
            glEnable(GL_BLEND);glBindTexture(GL_TEXTURE_2D,r->base);bind_vertices(r->full);glDrawArrays(GL_TRIANGLES,0,6);
        } else {
            GLuint tex;bool linear=false;
            if(batch->kind==DRAW_AMBIENT) {glUseProgram(r->background);glDisable(GL_BLEND);tex=r->ambient[batch->blade];}
            else {
                glEnable(GL_BLEND);Cached *c=find(r,r->plan.images[batch->image]);if(!c)return false;
                if(image_yuv(c->image.format)) {
                    glUseProgram(r->video);float values[16];video_colour_constants(c->image.format,values);constants(r,values,sizeof values);
                    glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,c->u);
                    glActiveTexture(GL_TEXTURE2);glBindTexture(GL_TEXTURE_2D,c->v);glActiveTexture(GL_TEXTURE0);
                } else if(batch->box_height) {
                    glUseProgram(r->cap);float values[]={(float)c->image.width,(float)c->image.height,(float)batch->box_height,0};constants(r,values,sizeof values);
                } else glUseProgram(r->ui);
                tex=c->texture;linear=!batch->box_height&&c->image.format!=IMAGE_MASK;
            }
            glBindTexture(GL_TEXTURE_2D,tex);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,linear?GL_LINEAR:GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,linear?GL_LINEAR:GL_NEAREST);
            bind_vertices(r->vbo);glDrawArrays(GL_TRIANGLES,(GLint)batch->first,(GLsizei)batch->count);
        }
    }
    if(readback) {
        glReadPixels(0,0,WIDTH,HEIGHT,GL_RGBA,GL_UNSIGNED_BYTE,r->pixels);
        for(int y=0;y<HEIGHT;y++)for(int x=0;x<WIDTH;x++) {
            const unsigned char *p=r->pixels+((HEIGHT-1-y)*WIDTH+x)*4;
            readback->pixels[y*WIDTH+x]=0xff000000|(uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2];
        }
    }
    GLenum error=glGetError();if(error){fprintf(stderr,"GPU validation GL error %04x\n",error);return false;}
    return true;
}
void gl_renderer_close(GlRenderer *r) {
    if(!r)return;
    if(r->display!=EGL_NO_DISPLAY) {
        if(r->context!=EGL_NO_CONTEXT) {
            /* Context destruction releases its buffers, programs and textures. */
            eglMakeCurrent(r->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroyContext(r->display,r->context);
        }
        if(r->surface!=EGL_NO_SURFACE)eglDestroySurface(r->display,r->surface);
        eglTerminate(r->display);
    }
    draw_list_close(&r->list);gpu_plan_close(&r->plan);free(r->pixels);free(r);
}
