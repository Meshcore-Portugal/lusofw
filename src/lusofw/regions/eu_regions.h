#pragma once

// Macro-polygon to quickly determine if a coordinate falls roughly within the European continent.
// This is intentionally coarse (10 points) to save memory and CPU cycles while providing
// enough accuracy to exclude North Africa and cover the Canary Islands and Azores.
static const GeoPoint poly_europe[] = {
    { 75.0f, -35.0f }, // NW (Iceland/Greenland sea)
    { 75.0f,  65.0f }, // NE (Ural mountains north)
    { 40.0f,  65.0f }, // SE (Caspian sea area)
    { 35.0f,  45.0f }, // East Med (Syria/Turkey border area)
    { 33.0f,  35.0f }, // Below Cyprus
    { 33.0f,  20.0f }, // Below Crete/Greece
    { 35.0f,  10.0f }, // Between Sicily and Tunisia
    { 35.0f,  -6.0f }, // Strait of Gibraltar
    { 27.0f, -20.0f }, // Below Canary Islands
    { 35.0f, -35.0f }, // SW (covers Azores and Madeira)
    { 75.0f, -35.0f }  // closing vertex (repeat of first)
};

static const RegionRing rings_europe[] = {
    {poly_europe, 11},
};

static const RegionPolygon REGION_EUROPE = { "#europe", rings_europe, 1 };
