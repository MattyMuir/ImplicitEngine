#include "Canvas.h"
#include "Main.h"

wxBEGIN_EVENT_TABLE(Canvas, wxPanel)
	EVT_PAINT(Canvas::OnPaint)
	EVT_SIZE(Canvas::Resized)
    EVT_MOUSEWHEEL(Canvas::OnScroll)
    EVT_MOTION(Canvas::OnMouseMove)
wxEND_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, int* attribs)
    : wxGLCanvas(parent, wxID_ANY, attribs)
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

    // Initialize renderer object
    renderer = new FilteringRenderer([&]() { this->JobProcessingFinished(); });
}

Canvas::~Canvas()
{
    delete context;
    delete vb;
    delete vbl;
    delete va;
    delete shader;

    delete renderer;
}

void Canvas::JobProcessingFinished()
{
    Refresh();
}

void Canvas::OnDraw()
{
    TIMER(frame);
    glViewport(0, 0, w, h);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing code here
    double world[] = { -0.5, -0.5, 0.0, 0.5, 0.5, -0.5 };
    float screen[sizeof(world) / sizeof(double)];

    for (int i = 0; i < sizeof(world) / sizeof(double) / 2; i++)
    {
        ToScreen(screen[i * 2], screen[i * 2 + 1], world[i * 2], world[i * 2 + 1]);
    }

    vb->SetData(screen, sizeof(screen));

    glDrawArrays(GL_TRIANGLES, 0, vb->Size());

    SwapBuffers();
    glFinish();
    STOP_LOG(frame);
}

void Canvas::OnPaint(wxPaintEvent& evt)
{
	GetSize(&w, &h);
    RecalculateBounds();

    OnDraw();
	evt.Skip();
}

void Canvas::Resized(wxSizeEvent& evt)
{
    if (w)
    {
        int newW, newH;
        GetSize(&newW, &newH);
        xOffset *= (double)w / newW;
        yOffset *= (double)h / newH;

        UpdateJobs();
    }
    
	evt.Skip();
}

void Canvas::OnScroll(wxMouseEvent& evt)
{
    // Zoom
    double factor = 1.5;
    if (evt.GetWheelRotation() < 0) { factor = 1.0 / factor; }

    double mouseX = evt.GetX();
    double mouseY = evt.GetY();

    xOffset += (factor - 1) * (2 * mouseX / w + xOffset - 1);
    yOffset += (factor - 1) * (-2 * mouseY / h + yOffset + 1);

    relXScale *= factor;
    relYScale *= factor;

    UpdateJobs();
    Refresh();
    evt.Skip();
}

void Canvas::OnMouseMove(wxMouseEvent& evt)
{
    if (!evt.LeftIsDown())
    {
        lastMouse.valid = false;
    }
    else
    {
        if (lastMouse.valid)
            OnMouseDrag((double)evt.GetX() - lastMouse.x, (double)evt.GetY() - lastMouse.y);

        lastMouse.valid = true;
        lastMouse.x = evt.GetX();
        lastMouse.y = evt.GetY();
    }
    evt.Skip();
}

void Canvas::OnMouseDrag(double delX, double delY)
{
    xOffset -= delX / w * 2;
    yOffset += delY / h * 2;

    UpdateJobs();
    Refresh();
}

void Canvas::ToScreen(float& xout, float& yout, double x, double y)
{
    xout = x * relXScale / w - xOffset;
    yout = y * relYScale / h - yOffset;
}

void Canvas::RecalculateBounds()
{
    bounds = { (double)w / relXScale * (xOffset - 1),
        (double)h / relYScale * (yOffset - 1),
        (double)w / relXScale * (xOffset + 1),
        (double)h / relYScale * (yOffset + 1) };
}

void Canvas::UpdateJobs()
{
    std::vector<Job*>& jobs = renderer->jobs;

    RecalculateBounds();
    for (Job* job : jobs)
    {
        job->bounds = bounds;
        job->status = JobStatus::OUTDATED;
    }
}