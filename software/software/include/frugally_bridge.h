#ifndef FRUGALLY_BRIDGE_H
#define FRUGALLY_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

int frugally_init(void);
float frugally_predict_rebar_spacing(float width_m, float length_m, int soil_type, float load_kn);
float frugally_predict_gravel_volume(float area_sqft, int soil_type);
float frugally_predict_formwork_bf(float perimeter_ft, float thickness_in);
void frugally_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FRUGALLY_BRIDGE_H */
