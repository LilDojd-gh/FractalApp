// Ids.h — идентификаторы элементов управления (общие для панели и главного окна).
#pragma once
#include <wx/defs.h>

enum ControlId {
    ID_RENDER = wxID_HIGHEST + 1,
    ID_SAVE,
    ID_RESET,
    ID_RESET_VIEW,
    ID_TYPE,
    ID_ITERATIONS,
    ID_ZOOM,
    ID_JULIA_RE,
    ID_JULIA_IM,
    ID_ROUGHNESS,
    ID_SEED,
    ID_GRID,
    ID_PALETTE,
    ID_INTERP,
    ID_INSIDE,
    ID_CYCLES,
    ID_MODE3D,
    ID_RENDER_MODE,
    ID_HEIGHT
};
