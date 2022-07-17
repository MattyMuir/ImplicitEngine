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
    Update();
    Refresh();
    Update();
}

void Canvas::DisplaySeeds(bool display)
{
    displaySeeds = display;
    renderer->KeepSeeds(display);
}

void Canvas::DisplayMeshes(bool display)
{
    displayMeshes = display;
    renderer->KeepMesh(display);
}

void Canvas::OnDraw()
{
    glViewport(0, 0, w, h);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Drawing code here
    for (auto job : renderer->jobs)
    {
        if (displaySeeds)
        {
            // Draw seeds
            auto seeds = renderer->GetSeeds(job->id);
            if (seeds.has_value())
                DrawSeeds(seeds.value().get());
        }

        if (displayMeshes)
        {
            // Draw filtering meshes
            auto mesh = renderer->GetMesh(job->id);
            if (mesh.has_value())
                DrawMesh(mesh.value().get());
        }

        /*std::vector<float> screenVerts;
        screenVerts.reserve(job->bufferedVerts.size());

        double xScale = relXScale / w;
        double yScale = relYScale / h;
        for (int i = 0; i < job->bufferedVerts.size() / 2; i++)
        {
            screenVerts.push_back(job->bufferedVerts[i * 2] * xScale - xOffset);
            screenVerts.push_back(job->bufferedVerts[i * 2 + 1] * yScale - yOffset);
        }

        vb->SetData(screenVerts.data(), screenVerts.size() * sizeof(float));
        glDrawArrays(GL_POINTS, 0, screenVerts.size() / 2);*/
    }

    SwapBuffers();
    glFinish();
}

void Canvas::OnPaint(wxPaintEvent& evt)
{
    RecalculateBounds();

    OnDraw();
	evt.Skip();
}

void Canvas::Resized(wxSizeEvent& evt)
{
    int newW, newH;
    GetSize(&newW, &newH);
    if (w)
    {
        xOffset *= (double)w / newW;
        yOffset *= (double)h / newH;
    }
    UpdateJobs();
    
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

void Canvas::DrawSeeds(Seeds* seeds)
{
    std::vector<float> screenSeeds;
    
    // Count seeds
    size_t num = 0;
    for (const auto& seedVec : *seeds)
        num += seedVec.size();

    screenSeeds.reserve(num * 2);

    // Transform seeds to screen coordinates
    double xScale = relXScale / w;
    double yScale = relYScale / h;
    for (const auto& seedVec : *seeds)
    {
        for (const Seed& s : seedVec)
        {
            screenSeeds.push_back(s.x * xScale - xOffset);
            screenSeeds.push_back(s.y * yScale - yOffset);
        }
    }

    vb->SetData(screenSeeds.data(), screenSeeds.size() * sizeof(float));
    glDrawArrays(GL_POINTS, 0, num);
}

void Canvas::DrawMesh(Mesh* mesh)
{
    struct Point
    {
        float x, y;
    };

    int dim = mesh->dim;
    std::vector<Point> verts;

    double boxW = mesh->bounds.w() / dim;
    double boxH = mesh->bounds.h() / dim;

    double xScale = relXScale / w;
    double yScale = relYScale / h;
    for (int by = 0; by < dim; by++)
    {
        for (int bx = 0; bx < dim; bx++)
        {
            if (!(*mesh).boxes[bx + by * dim]) continue;

            double x1 = mesh->bounds.xmin + boxW * bx;
            double x2 = x1 + boxW;

            double y1 = mesh->bounds.ymin + boxH * by;
            double y2 = y1 + boxH;

            // Transform point to screen coordinates
            float x1s = x1 * xScale - xOffset;
            float x2s = x2 * xScale - xOffset;
            float y1s = y1 * yScale - yOffset;
            float y2s = y2 * yScale - yOffset;

            Point corners[4] = { { x1s, y1s }, { x1s, y2s }, { x2s, y2s }, { x2s, y1s } };

            verts.push_back(corners[0]);
            verts.push_back(corners[1]);
            verts.push_back(corners[1]);
            verts.push_back(corners[2]);
            verts.push_back(corners[2]);
            verts.push_back(corners[3]);
            verts.push_back(corners[3]);
            verts.push_back(corners[0]);
        }
    }

    vb->SetData(verts.data(), verts.size() * sizeof(Point));
    glDrawArrays(GL_LINES, 0, verts.size());
}

void Canvas::RecalculateBounds()
{
    GetSize(&w, &h);

    bounds = { (double)w / relXScale * (xOffset - 1),
        (double)h / relYScale * (yOffset - 1),
        (double)w / relXScale * (xOffset + 1),
        (double)h / relYScale * (yOffset + 1) };
}

void Canvas::UpdateJobs()
{
    RecalculateBounds();
    for (auto job : renderer->jobs)
    {
        job->bounds = bounds;
        job->status = JobStatus::OUTDATED;
    }
}