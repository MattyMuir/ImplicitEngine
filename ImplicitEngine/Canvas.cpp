#include "Canvas.h"
#include "Main.h"

wxBEGIN_EVENT_TABLE(Canvas, wxPanel)
	EVT_PAINT(Canvas::OnPaint)
	EVT_SIZE(Canvas::Resized)
    EVT_IDLE(Canvas::OnIdle)
    EVT_KEY_DOWN(Canvas::OnKeyDown)
wxEND_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, int* attribs) : wxGLCanvas(parent, wxID_ANY, attribs)
{
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	mainPtr = (Main*)parent;

    // === GL Initialization ===
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.CoreProfile().OGLVersion(4, 6).Robust().ResetIsolation().EndList();
    context = new wxGLContext(this, nullptr, &ctxAttrs);

    while (!IsShown()) {};
    SetCurrent(*context);

    glLoadIdentity();
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Glew initialization failed.\n";
        exit(-1);
    }
    wglSwapIntervalEXT(0);

    // === Rendering Setup ===
    shader = new Shader(Shader::Compile("basic.shader"));
    shader->Bind();

    va = new VertexArray;
    vb = new VertexBuffer;
    vbl = new VertexBufferLayout;

    vbl->Push<float>(2);
    va->AddVBuffer(*vb, *vbl);

    SetWireframe(false);
}

Canvas::~Canvas()
{
    delete context;
    delete vb;
    delete vbl;
    delete va;
    delete shader;
}

void Canvas::OnDraw()
{
    TIMER(frame);
    glViewport(0, 0, w, h);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing code here
    float v[6] = { -0.5, -0.5, 0.0, 0.5, 0.5, -0.5 };
    vb->SetData(v, sizeof(v));

    glDrawArrays(GL_TRIANGLES, 0, vb->Size());

    SwapBuffers();
    glFinish();
    STOP_LOG(frame);
}

void Canvas::OnPaint(wxPaintEvent& evt)
{
	GetSize(&w, &h);
    OnDraw();
	evt.Skip();
}

void Canvas::Resized(wxSizeEvent& evt)
{
	evt.Skip();
}

void Canvas::OnIdle(wxIdleEvent& evt)
{
    evt.Skip();
}

void Canvas::OnKeyDown(wxKeyEvent& evt)
{
    if (evt.GetKeyCode() == WXK_SPACE)
    {
        // Keyboard input processing
    }
    evt.Skip();
}