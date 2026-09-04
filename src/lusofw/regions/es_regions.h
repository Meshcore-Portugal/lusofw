#pragma once

// SPAIN — DUMMY PLACEHOLDER.
// Minimal geometry that wires the per-country selection up for testing only.
// Replace every polygon here with real IGN/CAOP-grade data before release;
// the schema (COUNTRY_* tables, RegionRing/RegionPolygon) is final.

#include <Arduino.h>

#include "lusofw/AutoRegions.h"

#define COUNTRY_REGION_NAME "#es"
static const bool COUNTRY_IN_EUROPE = true;

#ifdef ENABLE_REGION_MACRO

// --- Macro Regiões (2, DUMMY) ---
static const GeoPoint poly_es_macro_norte_0[] PROGMEM = {
    {44.0, -9.0},
    {44.0,  3.0},
    {41.0,  3.0},
    {41.0, -9.0},
    {44.0, -9.0},
};

static const RegionRing rings_es_macro_norte[] PROGMEM = {
    {poly_es_macro_norte_0, 5},
};

static const GeoPoint poly_es_macro_sur_0[] PROGMEM = {
    {41.0, -9.0},
    {41.0,  0.0},
    {36.0,  0.0},
    {36.0, -9.0},
    {41.0, -9.0},
};

static const RegionRing rings_es_macro_sur[] PROGMEM = {
    {poly_es_macro_sur_0, 5},
};

static const RegionPolygon COUNTRY_MACRO_REGIONS[] = {
    {"#es-norte", rings_es_macro_norte, 1},
    {"#es-sur",   rings_es_macro_sur, 1},
};
static const int NUM_COUNTRY_MACRO_REGIONS = sizeof(COUNTRY_MACRO_REGIONS) / sizeof(COUNTRY_MACRO_REGIONS[0]);

#endif // ENABLE_REGION_MACRO

#ifdef ENABLE_REGION_DISTRICTS

// --- Distritos (2, DUMMY) ---
static const GeoPoint poly_es_districts_madrid_0[] PROGMEM = {
    {40.6, -4.0},
    {40.6, -3.4},
    {40.2, -3.4},
    {40.2, -4.0},
    {40.6, -4.0},
};

static const RegionRing rings_es_districts_madrid[] PROGMEM = {
    {poly_es_districts_madrid_0, 5},
};

static const GeoPoint poly_es_districts_barcelona_0[] PROGMEM = {
    {41.5, 2.0},
    {41.5, 2.4},
    {41.2, 2.4},
    {41.2, 2.0},
    {41.5, 2.0},
};

static const RegionRing rings_es_districts_barcelona[] PROGMEM = {
    {poly_es_districts_barcelona_0, 5},
};

static const RegionPolygon COUNTRY_DISTRICTS[] = {
    {"#es-madrid",    rings_es_districts_madrid, 1},
    {"#es-barcelona", rings_es_districts_barcelona, 1},
};
static const int NUM_COUNTRY_DISTRICTS = sizeof(COUNTRY_DISTRICTS) / sizeof(COUNTRY_DISTRICTS[0]);

#endif // ENABLE_REGION_DISTRICTS

// No-GPS fallback: node-name prefix -> ONE district + ONE macro region (DUMMY)
static const FallbackRegion fallback_ES_MA[] = {
    {"#es-madrid", KIND_DISTRICT},
    {"#es-sur",    KIND_MACRO},
};

static const FallbackRegion fallback_ES_BA[] = {
    {"#es-barcelona", KIND_DISTRICT},
    {"#es-norte",     KIND_MACRO},
};

static const RegionFallback COUNTRY_FALLBACK_REGIONS[] = {
    {"MA", fallback_ES_MA, sizeof(fallback_ES_MA) / sizeof(fallback_ES_MA[0])},
    {"BA", fallback_ES_BA, sizeof(fallback_ES_BA) / sizeof(fallback_ES_BA[0])},
};
static const int NUM_COUNTRY_FALLBACK_REGIONS = sizeof(COUNTRY_FALLBACK_REGIONS) / sizeof(COUNTRY_FALLBACK_REGIONS[0]);
