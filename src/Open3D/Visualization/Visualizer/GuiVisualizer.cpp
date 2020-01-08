// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "GuiVisualizer.h"

#include "Open3D/Open3DConfig.h"
#include "Open3D/Geometry/BoundingVolume.h"
#include "Open3D/Geometry/Geometry3D.h"
#include "Open3D/GUI/Application.h"
#include "Open3D/GUI/Button.h"
#include "Open3D/GUI/Color.h"
#include "Open3D/GUI/Dialog.h"
#include "Open3D/GUI/Label.h"
#include "Open3D/GUI/Layout.h"
#include "Open3D/GUI/SceneWidget.h"
#include "Open3D/GUI/Theme.h"
#include "Open3D/Utility/Console.h"
#include "Open3D/Visualization/Rendering/Camera.h"
#include "Open3D/Visualization/Rendering/CameraManipulator.h"
#include "Open3D/Visualization/Rendering/RendererStructs.h"
#include "Open3D/Visualization/Rendering/Scene.h"

#if !defined(WIN32)
#    include <unistd.h>
#else
#    include <io.h>
#endif
#include <fcntl.h>

namespace open3d {
namespace visualization {

namespace {
std::string getIOErrorString(errno_t errnoVal) {
    switch (errnoVal) {
        case EACCES:
            return "Permission denied";
        case EDQUOT:
            return "Disk quota exceeded";
        case EEXIST:
            return "File already exists";
        case ELOOP:
            return "Too many symlinks in path";
        case EMFILE:
            return "Process has no more file descriptors available";
        case ENAMETOOLONG:
            return "Filename is too long";
        case ENFILE:
        case ENOSPC:
            return "File system is full";
        case ENOENT:
            return "File does not exist";
        default: {
            std::stringstream s;
            s << "Error " << errnoVal << " while opening file";
            return s.str();
        }
    }
}

bool readBinaryFile(const std::string& path, std::vector<char> *bytes,
                    std::string *errorStr)
{
    bytes->clear();
    if (errorStr) {
        *errorStr = "";
    }

    // Open file
    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        if (errorStr) {
            *errorStr = getIOErrorString(errno);
        }
        return false;
    }

    // Get file size
    size_t filesize = (size_t)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);  // reset file pointer back to beginning

    // Read data
    bytes->resize(filesize);
    read(fd, bytes->data(), filesize);

    // We're done, close and return
    close(fd);
    return true;
}

std::shared_ptr<gui::Dialog> createAboutDialog(gui::Window *window)
{
    auto &theme = window->GetTheme();
    auto dlg = std::make_shared<gui::Dialog>("About");

    auto title = std::make_shared<gui::Label>((std::string("Open3D ") + OPEN3D_VERSION).c_str());
    auto text = std::make_shared<gui::Label>(
        "The MIT License (MIT)\n"
        "Copyright (c) 2018 www.open3d.org\n\n"

        "Permission is hereby granted, free of charge, to any person obtaining "
        "a copy of this software and associated documentation files (the "
        "\"Software\"), to deal in the Software without restriction, including "
        "without limitation the rights to use, copy, modify, merge, publish, "
        "distribute, sublicense, and/or sell copies of the Software, and to "
        "permit persons to whom the Software is furnished to do so, subject to "
        "the following conditions:\n\n"

        "The above copyright notice and this permission notice shall be "
        "included in all copies or substantial portions of the Software.\n\n"

        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
        "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
        "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. "
        "IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY "
        "CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, "
        "TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE "
        "SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");
    auto ok = std::make_shared<gui::Button>("OK");
    ok->SetOnClicked([window]() {
        window->CloseDialog();
    });

    gui::Margins margins(theme.fontSize);
    auto layout = std::make_shared<gui::Vert>(0, margins);
    layout->AddChild(gui::Horiz::MakeCentered(title));
    layout->AddChild(gui::Horiz::MakeFixed(theme.fontSize));
    layout->AddChild(text);
    layout->AddChild(gui::Horiz::MakeFixed(theme.fontSize));
    layout->AddChild(gui::Horiz::MakeCentered(ok));
    dlg->AddChild(layout);

    return dlg;
}

std::shared_ptr<gui::Dialog> createContactDialog(gui::Window *window)
{
    auto &theme = window->GetTheme();
    auto em = theme.fontSize;
    auto dlg = std::make_shared<gui::Dialog>("Contact Us");

    auto title = std::make_shared<gui::Label>("Contact Us");
    auto leftCol = std::make_shared<gui::Label>("Web site:\n"
                                                "Code:\n"
                                                "Mailing list:\n"
                                                "Discord channel:");
    auto rightCol = std::make_shared<gui::Label>(
                            "http://www.open3d.org\n"
                            "http://github.org/intel-isl/Open3D\n"
                            "http://www.open3d.org/index.php/subscribe/\n"
                            "https://discord.gg/D35BGvn");
    auto ok = std::make_shared<gui::Button>("OK");
    ok->SetOnClicked([window]() {
        window->CloseDialog();
    });

    gui::Margins margins(em);
    auto layout = std::make_shared<gui::Vert>(0, margins);
    layout->AddChild(gui::Horiz::MakeCentered(title));
    layout->AddChild(gui::Horiz::MakeFixed(em));

    auto columns = std::make_shared<gui::Horiz>(em, gui::Margins());
    columns->AddChild(leftCol);
    columns->AddChild(rightCol);
    layout->AddChild(columns);

    layout->AddChild(gui::Horiz::MakeFixed(em));
    layout->AddChild(gui::Horiz::MakeCentered(ok));
    dlg->AddChild(layout);

    return dlg;
}

}

enum MenuId { FILE_OPEN, FILE_SAVE, FILE_CLOSE,
              VIEW_POINTS, VIEW_WIREFRAME, VIEW_MESH,
              HELP_ABOUT, HELP_CONTACT };

struct GuiVisualizer::Impl {
    std::shared_ptr<gui::SceneWidget> scene;
    std::shared_ptr<gui::Horiz> bottomBar;
};

GuiVisualizer::GuiVisualizer(const std::vector<std::shared_ptr<const geometry::Geometry>>& geometries,
                             const std::string &title,
                             int width, int height, int left, int top)
: gui::Window(title, left, top, width, height)
, impl_(new GuiVisualizer::Impl())
{
    auto &app = gui::Application::GetInstance();
    auto &theme = GetTheme();

    // Create material
    visualization::MaterialHandle nonmetal;
    std::string err;
    std::string rsrcPath = app.GetResourcePath();
    std::string path = rsrcPath + "/nonmetal.filamat";
    std::vector<char> bytes;
    if (readBinaryFile(path, &bytes, &err)) {
        nonmetal = GetRenderer().AddMaterial(bytes.data(), bytes.size());
    } else {
        utility::LogWarning((std::string("Error opening ") + path + ":" + err).c_str());
    }
    auto white = GetRenderer().ModifyMaterial(nonmetal)
            .SetColor("baseColor", {1.0, 1.0, 1.0})
            .SetParameter("roughness", 0.5f)
            .SetParameter("clearCoat", 1.f)
            .SetParameter("clearCoatRoughness", 0.3f)
            .Finish();

    // Create scene
    auto sceneId = GetRenderer().CreateScene();
    auto scene = std::make_shared<gui::SceneWidget>(*GetRenderer().GetScene(sceneId));
    impl_->scene = scene;
    scene->SetBackgroundColor(gui::Color(1.0, 1.0, 1.0));

    // Create light
    visualization::LightDescription lightDescription;
    lightDescription.intensity = 100000;
    lightDescription.direction = { -0.707, -.707, 0.0 };
    lightDescription.customAttributes["custom_type"] = "SUN";

    scene->GetScene()->AddLight(lightDescription);

    // Add geometries
    geometry::AxisAlignedBoundingBox bounds;
    for (auto &g : geometries) {
        switch (g->GetGeometryType()) {
            case geometry::Geometry::GeometryType::OrientedBoundingBox:
            case geometry::Geometry::GeometryType::AxisAlignedBoundingBox:
            case geometry::Geometry::GeometryType::PointCloud:
            case geometry::Geometry::GeometryType::LineSet:
            case geometry::Geometry::GeometryType::MeshBase:
            case geometry::Geometry::GeometryType::TriangleMesh:
            case geometry::Geometry::GeometryType::HalfEdgeTriangleMesh:
            case geometry::Geometry::GeometryType::TetraMesh:
            case geometry::Geometry::GeometryType::Octree:
            case geometry::Geometry::GeometryType::VoxelGrid: {
                auto g3 = std::static_pointer_cast<const geometry::Geometry3D>(g);
                bounds += g3->GetAxisAlignedBoundingBox();
                scene->GetScene()->AddGeometry(*g3, white);
            }
            case geometry::Geometry::GeometryType::RGBDImage:
            case geometry::Geometry::GeometryType::Image:
            case geometry::Geometry::GeometryType::Unspecified:
                break;
        }
    }

    // Setup camera
    scene->GetCameraManipulator()->SetFov(90.0f);
    scene->GetCameraManipulator()->SetNearPlane(0.1f);
    scene->GetCameraManipulator()->SetFarPlane(2.0 * bounds.GetExtent().norm());
    auto boundsMin = bounds.GetMinBound();
    auto boundsMax = bounds.GetMaxBound();
    auto boundsMid = Eigen::Vector3f((boundsMin.x() + boundsMax.x()) / 2.0,
                                     (boundsMin.y() + boundsMax.y()) / 2.0,
                                     (boundsMin.z() + boundsMax.z()) / 2.0);
    Eigen::Vector3f farthest(std::max(std::abs(boundsMin.x()),
                                      std::abs(boundsMax.x())),
                             std::max(std::abs(boundsMin.y()),
                                      std::abs(boundsMax.y())),
                             std::max(std::abs(boundsMin.z()),
                                      std::abs(boundsMax.z())));
    scene->GetCameraManipulator()->LookAt({0, 0, 0}, farthest);

    // Setup UI
    int spacing = std::max(1, int(std::ceil(0.25 * theme.fontSize)));

    auto buttonRegular = std::make_shared<gui::Button>("Default camera");
    auto buttonTop = std::make_shared<gui::Button>("Top");
    buttonTop->SetOnClicked([scene, boundsMid, boundsMax]() {
        Eigen::Vector3f eye(boundsMid.x(), 1.5 * boundsMax.y(), boundsMid.z());
        Eigen::Vector3f up(1, 0, 0);
        scene->GetCameraManipulator()->LookAt(boundsMid, eye, up);
    });
    auto buttonFront = std::make_shared<gui::Button>("Front");
    buttonFront->SetOnClicked([scene, boundsMid, boundsMax]() {
        Eigen::Vector3f eye(1.5 * boundsMax.x(), boundsMid.y(), boundsMid.z());
        scene->GetCameraManipulator()->LookAt(boundsMid, eye);
    });
    auto buttonSide = std::make_shared<gui::Button>("Side");
    buttonSide->SetOnClicked([scene, boundsMid, boundsMax]() {
        Eigen::Vector3f eye(boundsMid.x(), boundsMid.y(), 1.5 * boundsMax.z());
        scene->GetCameraManipulator()->LookAt(boundsMid, eye);
    });
    auto bottomBar = std::make_shared<gui::Horiz>(spacing,
                                                  gui::Margins(0, spacing));
    impl_->bottomBar = bottomBar;
    bottomBar->SetBackgroundColor(gui::Color(0, 0, 0, 0.5));
    bottomBar->AddChild(gui::Horiz::MakeStretch());
    bottomBar->AddChild(buttonRegular);
    bottomBar->AddChild(buttonTop);
    bottomBar->AddChild(buttonFront);
    bottomBar->AddChild(buttonSide);
    bottomBar->AddChild(gui::Horiz::MakeStretch());

    AddChild(scene);
    AddChild(bottomBar);

    // Create menu
    auto fileMenu = std::make_shared<gui::Menu>();
    fileMenu->AddItem("Open", "Ctrl-O", FILE_OPEN);
    fileMenu->AddItem("Save", "Ctrl-S", FILE_SAVE);
    fileMenu->AddSeparator();
    fileMenu->AddItem("Close", "Ctrl-W", FILE_CLOSE);
    auto viewMenu = std::make_shared<gui::Menu>();
    viewMenu->AddItem("Points", nullptr, VIEW_POINTS);
    viewMenu->AddItem("Wireframe", nullptr, VIEW_WIREFRAME);
    viewMenu->AddItem("Mesh", nullptr, VIEW_MESH);
    auto helpMenu = std::make_shared<gui::Menu>();
    helpMenu->AddItem("About", nullptr, HELP_ABOUT);
    helpMenu->AddItem("Contact", nullptr, HELP_CONTACT);
    auto menu = std::make_shared<gui::Menu>();
    menu->AddMenu("File", fileMenu);
    menu->AddMenu("View", viewMenu);
    menu->AddMenu("Help", helpMenu);
    this->SetMenubar(menu);
}

GuiVisualizer::~GuiVisualizer() {
}

void GuiVisualizer::Layout(const gui::Theme& theme) {
    auto r = GetContentRect();
    impl_->scene->SetFrame(r);

    auto bottomHeight = impl_->bottomBar->CalcPreferredSize(theme).height;
    gui::Rect bottomRect(0, r.GetBottom() - bottomHeight,
                         r.width, bottomHeight);
    impl_->bottomBar->SetFrame(bottomRect);

    Super::Layout(theme);
}

void GuiVisualizer::OnMenuItemSelected(gui::Menu::ItemId itemId) {
    switch (MenuId(itemId)) {
        case FILE_OPEN:
            break;
        case FILE_SAVE:
            break;
        case FILE_CLOSE:
            this->Close();
            break;
        case VIEW_POINTS:
            break;
        case VIEW_WIREFRAME:
            break;
        case VIEW_MESH:
            break;
        case HELP_ABOUT: {
            auto dlg = createAboutDialog(this);
            ShowDialog(dlg);
            break;
        }
        case HELP_CONTACT: {
            auto dlg = createContactDialog(this);
            ShowDialog(dlg);
            break;
        }
    }
}

} // visualizer
} // open3d
