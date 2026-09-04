#pragma once

// BRAZIL — DUMMY PLACEHOLDER.
// Minimal geometry that wires the per-country selection up for testing only.
// Replace every polygon here with real IBGE-grade data before release;
// the schema (country-prefixed tables, RegionRing/RegionPolygon) is final.
// Note: BR_IN_EUROPE is false — no EU duty-cycle/tx-power limits apply.

#include <Arduino.h>

#include "lusofw/AutoRegions.h"

#define BR_REGION_NAME "#br"
static const bool BR_IN_EUROPE = false;

#ifdef ENABLE_REGION_MACRO

// --- Macro Regiões (2, DUMMY) ---
static const GeoPoint poly_br_macro_norte_0[] PROGMEM = {
    {  0.0, -70.0},
    {  0.0, -50.0},
    {-10.0, -50.0},
    {-10.0, -70.0},
    {  0.0, -70.0},
};

static const RegionRing rings_br_macro_norte[] PROGMEM = {
    {poly_br_macro_norte_0, 5},
};

static const GeoPoint poly_br_macro_sudeste_0[] PROGMEM = {
    {-15.0, -55.0},
    {-15.0, -40.0},
    {-25.0, -40.0},
    {-25.0, -55.0},
    {-15.0, -55.0},
};

static const RegionRing rings_br_macro_sudeste[] PROGMEM = {
    {poly_br_macro_sudeste_0, 5},
};

static const RegionPolygon BR_MACRO_REGIONS[] = {
    {"#br-norte",   rings_br_macro_norte, 1},
    {"#br-sudeste", rings_br_macro_sudeste, 1},
};
static const int NUM_BR_MACRO_REGIONS = sizeof(BR_MACRO_REGIONS) / sizeof(BR_MACRO_REGIONS[0]);

#else
static const RegionPolygon* const BR_MACRO_REGIONS = nullptr;
static const int NUM_BR_MACRO_REGIONS = 0;
#endif // ENABLE_REGION_MACRO

#ifdef ENABLE_REGION_DISTRICTS

// --- Distritos / estados (2, DUMMY) ---
static const GeoPoint poly_br_districts_sao_paulo_0[] PROGMEM = {
    {-23.3, -46.9},
    {-23.3, -46.3},
    {-23.8, -46.3},
    {-23.8, -46.9},
    {-23.3, -46.9},
};

static const RegionRing rings_br_districts_sao_paulo[] PROGMEM = {
    {poly_br_districts_sao_paulo_0, 5},
};

static const GeoPoint poly_br_districts_rio_de_janeiro_0[] PROGMEM = {
    {-22.8, -43.5},
    {-22.8, -43.0},
    {-23.2, -43.0},
    {-23.2, -43.5},
    {-22.8, -43.5},
};

static const RegionRing rings_br_districts_rio_de_janeiro[] PROGMEM = {
    {poly_br_districts_rio_de_janeiro_0, 5},
};

static const RegionPolygon BR_DISTRICTS[] = {
    {"#br-sao-paulo",       rings_br_districts_sao_paulo, 1},
    {"#br-rio-de-janeiro",  rings_br_districts_rio_de_janeiro, 1},
};
static const int NUM_BR_DISTRICTS = sizeof(BR_DISTRICTS) / sizeof(BR_DISTRICTS[0]);

#else
static const RegionPolygon* const BR_DISTRICTS = nullptr;
static const int NUM_BR_DISTRICTS = 0;
#endif // ENABLE_REGION_DISTRICTS

// No-GPS fallback: node-name prefix -> ONE district + ONE macro region (DUMMY)
static const FallbackRegion fallback_BR_SP[] = {
    {"#br-sao-paulo", KIND_DISTRICT},
    {"#br-sudeste",   KIND_MACRO},
};

static const FallbackRegion fallback_BR_RJ[] = {
    {"#br-rio-de-janeiro", KIND_DISTRICT},
    {"#br-sudeste",        KIND_MACRO},
};

static const RegionFallback BR_FALLBACK_REGIONS[] = {
    {"SP", fallback_BR_SP, sizeof(fallback_BR_SP) / sizeof(fallback_BR_SP[0])},
    {"RJ", fallback_BR_RJ, sizeof(fallback_BR_RJ) / sizeof(fallback_BR_RJ[0])},
};
static const int NUM_BR_FALLBACK_REGIONS = sizeof(BR_FALLBACK_REGIONS) / sizeof(BR_FALLBACK_REGIONS[0]);
