#pragma once

// Démarre le client PogHome après l'initialisation de mDNS. L'adoption et le
// bus MQTT tournent dans une tâche dédiée afin de ne jamais bloquer les LED.
void pogdevBegin();

// Demande une publication immédiate après une modification locale ou web.
void pogdevNotifyState();

// Republie aussi la description des entites (sections, noms et utilites).
void pogdevNotifyConfig();
