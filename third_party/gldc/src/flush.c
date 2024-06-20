#include <stdbool.h>
#include "gldc.h"

PolyList OP_LIST;
PolyList PT_LIST;
PolyList TR_LIST;

void glKosInit() {
    _glInitContext();
    _glInitTextures();

    OP_LIST.list_type = PVR_LIST_OP_POLY;
    PT_LIST.list_type = PVR_LIST_PT_POLY;
    TR_LIST.list_type = PVR_LIST_TR_POLY;

    aligned_vector_init(&OP_LIST.vector);
    aligned_vector_init(&PT_LIST.vector);
    aligned_vector_init(&TR_LIST.vector);

    aligned_vector_reserve(&OP_LIST.vector, 1024 * 3);
    aligned_vector_reserve(&PT_LIST.vector,  512 * 3);
    aligned_vector_reserve(&TR_LIST.vector, 1024 * 3);
}


void glKosSwapBuffers() {
    _glApplyScissor(true);

    pvr_scene_begin();   
        if(OP_LIST.vector.size > 2) {
            pvr_list_begin(PVR_LIST_OP_POLY);
            SceneListSubmit((Vertex*)OP_LIST.vector.data, OP_LIST.vector.size);
            pvr_list_finish();
        }

        if(PT_LIST.vector.size > 2) {
            pvr_list_begin(PVR_LIST_PT_POLY);
            SceneListSubmit((Vertex*)PT_LIST.vector.data, PT_LIST.vector.size);
            pvr_list_finish();
        }

        if(TR_LIST.vector.size > 2) {
            pvr_list_begin(PVR_LIST_TR_POLY);
            SceneListSubmit((Vertex*)TR_LIST.vector.data, TR_LIST.vector.size);
            pvr_list_finish();
        }
    pvr_scene_finish();
    
    OP_LIST.vector.size = 0;
    PT_LIST.vector.size = 0;
    TR_LIST.vector.size = 0;
}
