#pragma once

#include <Arduino.h>

struct RegionFallback {
    const char* prefix;
    const char** regions;
    int num_regions;
};

// Define region string arrays (Apenas Distrito alvo + ANEPC NUTS II + ANEPC CIM + IATA)
const char* fallback_AV[] = {"#pt-aveiro", "#pt-iata-opo", "#pt-centro", "#pt-regiao-de-aveiro"};
const char* fallback_AC[] = {"#pt-acores", "#pt-iata-pdl", "#pt-acores"};
const char* fallback_BE[] = {"#pt-beja", "#pt-iata-byj", "#pt-alentejo", "#pt-baixo-alentejo"};
const char* fallback_BR[] = {"#pt-braga", "#pt-iata-opo", "#pt-norte", "#pt-cavado", "#pt-ave"};
const char* fallback_BA[] = {"#pt-braganca", "#pt-iata-opo", "#pt-norte", "#pt-terras-tras-os-montes"};
const char* fallback_CB[] = {"#pt-castelo-branco", "#pt-iata-lis", "#pt-centro", "#pt-beira-baixa"};
const char* fallback_CO[] = {"#pt-coimbra", "#pt-iata-opo", "#pt-centro", "#pt-regiao-de-coimbra"};
const char* fallback_FA[] = {"#pt-faro", "#pt-iata-fao", "#pt-algarve"};
const char* fallback_GU[] = {"#pt-guarda", "#pt-iata-opo", "#pt-centro", "#pt-beiras-e-serra-estrela"};
const char* fallback_LE[] = {"#pt-leiria", "#pt-iata-lis", "#pt-centro", "#pt-lisboa-vale-do-tejo", "#pt-regiao-de-leiria", "#pt-oeste"};
const char* fallback_LI[] = {"#pt-lisboa", "#pt-iata-lis", "#pt-lisboa-vale-do-tejo", "#pt-grande-lisboa", "#pt-oeste"};
const char* fallback_MA[] = {"#pt-madeira", "#pt-iata-fnc", "#pt-madeira"};
const char* fallback_PA[] = {"#pt-portalegre", "#pt-iata-byj", "#pt-alentejo", "#pt-alto-alentejo"};
const char* fallback_PO[] = {"#pt-porto", "#pt-iata-opo", "#pt-norte", "#pt-porto", "#pt-tamega-e-sousa"};
const char* fallback_SA[] = {"#pt-santarem", "#pt-iata-lis", "#pt-lisboa-vale-do-tejo", "#pt-centro", "#pt-medio-tejo", "#pt-leziria-do-tejo"};
const char* fallback_SE[] = {"#pt-setubal", "#pt-iata-lis", "#pt-lisboa-vale-do-tejo", "#pt-alentejo", "#pt-peninsula-de-setubal", "#pt-alentejo-litoral"};
const char* fallback_VC[] = {"#pt-viana-do-castelo", "#pt-iata-opo", "#pt-norte", "#pt-alto-minho"};
const char* fallback_VR[] = {"#pt-vila-real", "#pt-iata-opo", "#pt-norte", "#pt-douro"};
const char* fallback_VI[] = {"#pt-viseu", "#pt-iata-opo", "#pt-centro", "#pt-norte", "#pt-viseu-dao-lafoes", "#pt-douro"};
const char* fallback_EV[] = {"#pt-evora", "#pt-iata-byj", "#pt-alentejo", "#pt-alentejo-central"};

const RegionFallback FALLBACK_REGIONS[] = {
    {"AV", fallback_AV, 4},
    {"AC", fallback_AC, 3},
    {"BE", fallback_BE, 4},
    {"BR", fallback_BR, 5},
    {"BA", fallback_BA, 4},
    {"CB", fallback_CB, 4},
    {"CO", fallback_CO, 4},
    {"FA", fallback_FA, 3},
    {"GU", fallback_GU, 4},
    {"LE", fallback_LE, 6},
    {"LI", fallback_LI, 5},
    {"MA", fallback_MA, 3},
    {"PA", fallback_PA, 4},
    {"PO", fallback_PO, 5},
    {"SA", fallback_SA, 6},
    {"SE", fallback_SE, 6},
    {"VC", fallback_VC, 4},
    {"VR", fallback_VR, 4},
    {"VI", fallback_VI, 6},
    {"EV", fallback_EV, 4}
};
const int NUM_FALLBACK_REGIONS = 20;
