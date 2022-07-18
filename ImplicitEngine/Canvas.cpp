#include "Canvas.h"
#include "Main.h"

wxBEGIN_EVENT_TABLE(Canvas, wxPanel)
	EVT_PAINT(Canvas::OnPaint)
	EVT_SIZE(Canvas::Resized)
    EVT_MOUSEWHEEL(Canvas::OnScroll)
    EVT_MOTION(Canvas::OnMouseMove)
wxEND_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, const wxGLAttributes& attribs)
    : wxGLCanvas(parent, attribs)
{
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	mainPtr = (Main*)parent;

    // === GL Initialization ===
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().OGLVersion(4, 6).ResetIsolation();
#ifdef _DEBUG
    ctxAttrs.DebugCtx();
#else
    ctxAttrs.Robust();
#endif
    ctxAttrs.EndList();
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

    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // === Rendering Setup ===
    shader = new Shader(Shader::Compile("basic.shader"));

    va = new VertexArray;
    vb = new VertexBuffer;
    vbl = new VertexBufferLayout;

    vbl->Push<float>(2);
    va->AddVBuffer(*vb, *vbl);

    // Initialize text renderer
    textRenderer = new TextRenderer;
    textRenderer->LoadFont("C:\\Windows\\Fonts\\Arial.ttf", "Arial", 48);

    shader->Bind();
    va->Bind();

    // Initialize function renderer object
    renderer = new FilteringRenderer([&]() { this->JobProcessingFinished(); });
}

Canvas::~Canvas()
{
    delete context;
    delete vb;
    delete vbl;
    delete va;
    delete shader;
    delete textRenderer;

    delete renderer;
}

void Canvas::JobProcessingFinished()
{
    Update();
    Refresh();
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
    DrawGrid();

    for (auto job : renderer->jobs)
    {
        if (displaySeeds)
        {
            // Draw seeds
            auto seeds = renderer->GetSeeds(job->id);
            if (seeds.has_value())
                DrawSeeds(seeds.value());
        }

        if (displayMeshes)
        {
            // Draw filtering meshes
            auto mesh = renderer->GetMesh(job->id);
            if (mesh.has_value())
                DrawMesh(mesh.value());
        }
    }

    SwapBuffers();
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
    double factor = 1.1;
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

void Canvas::DrawGrid()
{
    float xminS = bounds.xmin * relXScale / w - xOffset;
    float yminS = bounds.ymin * relYScale / h - yOffset;
    float xmaxS = bounds.xmax * relXScale / w - xOffset;
    float ymaxS = bounds.ymax * relYScale / h - yOffset;

    float xzeroS = -xOffset;
    float yzeroS = -yOffset;

    // === Draw zero axes ===
    //std::array<float, 8> axisBuf = { xminS, yzeroS, xmaxS, yzeroS, xzeroS, yminS, xzeroS, ymaxS };

    float lineW = 2.0 / w;
    float lineH = 2.0 / h;

    Point p[8] = {     { xminS, yzeroS - lineH }, { xminS, yzeroS + lineH },
                            { xmaxS, yzeroS - lineH }, { xmaxS, yzeroS + lineH },
                            { xzeroS - lineW, yminS }, { xzeroS + lineW, yminS },
                            { xzeroS - lineW, ymaxS }, { xzeroS + lineW, ymaxS } };
    std::array<Point, 12> axisBuf = { p[0], p[1], p[2], p[1], p[2], p[3],
        p[4], p[5], p[6], p[5], p[6], p[7] };

    vb->SetData(axisBuf.data(), axisBuf.size() * sizeof(Point));

    glUniform4f(shader->GetUniformLocation("col"), 0.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 12);

    // === Draw major gridlines ===
    wxDisplay display(wxDisplay::GetFromWindow(this));
    wxRect screen = display.GetClientArea();

    double targetMajorSize = std::max(screen.width, screen.height) / 15;

    // Find grid width that achieves target
    double exactGridW = targetMajorSize / w * bounds.w();
    auto [mantissa, exponent] = RoundMajorGridValue(exactGridW);
    double gridW = pow(10, exponent) * mantissa;

    DrawGridlines(gridW, 0.3f);

    // === Draw minor gridlines ===
    DrawGridlines(gridW / 5, 0.1f);

    // === Draw axis text ===
    textRenderer->RenderText("Hello", { "Arial", 48 }, w, h, w / 2, h / 2, 1.0f);
    shader->Bind();
    va->Bind();
}

void Canvas::DrawGridlines(double spacing, float opacity)
{
    float xminS = bounds.xmin * relXScale / w - xOffset;
    float yminS = bounds.ymin * relYScale / h - yOffset;
    float xmaxS = bounds.xmax * relXScale / w - xOffset;
    float ymaxS = bounds.ymax * relYScale / h - yOffset;

    double startY = ceil(bounds.ymin / spacing) * spacing;
    int num = bounds.h() / spacing;

    std::vector<Point> majorsBuf;
    for (int yi = 0; yi <= num; yi++)
    {
        double worldY = startY + spacing * yi;
        float screenY = worldY * relYScale / h - yOffset;

        majorsBuf.push_back({ xminS, screenY });
        majorsBuf.push_back({ xmaxS, screenY });
    }

    // Vertical lines
    double startX = ceil(bounds.xmin / spacing) * spacing;
    num = bounds.w() / spacing;

    for (int xi = 0; xi <= num; xi++)
    {
        double worldX = startX + spacing * xi;
        float screenX = worldX * relXScale / w - xOffset;

        majorsBuf.push_back({ screenX, yminS });
        majorsBuf.push_back({ screenX, ymaxS });
    }

    vb->SetData(majorsBuf.data(), majorsBuf.size() * sizeof(Point));

    glUniform4f(shader->GetUniformLocation("col"), 0.0f, 0.0f, 0.0f, opacity);
    glDrawArrays(GL_LINES, 0, majorsBuf.size());
}

std::pair<int, int> Canvas::RoundMajorGridValue(double val)
{
    int exponent = std::floor(std::log(val) / std::log(10));
    double pow = std::pow(10, exponent);

    double mantissa = val / std::pow(10, exponent);

    double v[] = { 5 * pow / 10, pow, 2 * pow, 5 * pow, 10 * pow };
    std::array<double, 5> d;
    for (int i = 0; i < 5; i++)
        d[i] = std::abs(val - v[i]);

    int best = std::distance(d.begin(), std::min_element(d.begin(), d.end()));

    std::pair<int, int> options[] = { { 5, exponent - 1 }, { 1, exponent }, { 2, exponent }, { 5, exponent }, { 1, exponent + 1 } };
    return options[best];
}

void Canvas::DrawSeeds(const std::shared_ptr<Seeds>& seeds)
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

    glUniform4f(shader->GetUniformLocation("col"), 0.0f, 0.0f, 0.0f, 1.0f);
    vb->SetData(screenSeeds.data(), screenSeeds.size() * sizeof(float));
    glDrawArrays(GL_POINTS, 0, num);
}

void Canvas::DrawMesh(const std::shared_ptr<Mesh>& mesh)
{
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
            verts.push_back(corners[2]);
            verts.push_back(corners[2]);
            verts.push_back(corners[3]);
            verts.push_back(corners[0]);
        }
    }

    glUniform4f(shader->GetUniformLocation("col"), 1.0f, 0.0f, 0.0f, 0.2f);
    vb->SetData(verts.data(), verts.size() * sizeof(Point));
    glDrawArrays(GL_TRIANGLES, 0, verts.size());
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