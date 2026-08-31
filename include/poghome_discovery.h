#pragma once

#include <cstdint>

namespace pogdev {

// Vue minimale et indépendante d'Arduino d'un résultat _poghome._tcp. Le
// protocole 0 représente une ancienne annonce sans TXT `proto`; elle reste un
// repli compatible. Toute version explicite autre que 1 est refusée.
struct PogHomeCandidate {
  uint32_t ipv4 = 0;
  uint16_t apiPort = 0;
  uint8_t protocol = 0;
};

inline bool pogHomeCandidateCompatible(const PogHomeCandidate &candidate) {
  return candidate.ipv4 != 0 && candidate.apiPort != 0 &&
         (candidate.protocol == 0 || candidate.protocol == 1);
}

inline bool pogHomeCandidateSameSubnet(const PogHomeCandidate &candidate,
                                       uint32_t localIpv4,
                                       uint32_t subnetMask) {
  return localIpv4 != 0 && subnetMask != 0 &&
         (candidate.ipv4 & subnetMask) == (localIpv4 & subnetMask);
}

inline uint8_t pogHomeCandidateRank(const PogHomeCandidate &candidate,
                                    uint32_t localIpv4,
                                    uint32_t subnetMask) {
  // Le sous-réseau local domine : un ancien POG Home encore annoncé par une
  // interface VPN ou un pont ne doit plus détourner une lampe du foyer.
  uint8_t rank = pogHomeCandidateSameSubnet(candidate, localIpv4, subnetMask)
                     ? 2
                     : 0;
  if (candidate.protocol == 1) ++rank;
  return rank;
}

// Comparateur total : à qualité égale, l'adresse puis le port croissants
// rendent le choix indépendant de l'ordre non garanti des réponses mDNS.
inline bool pogHomeCandidateBetter(const PogHomeCandidate &candidate,
                                   const PogHomeCandidate &current,
                                   uint32_t localIpv4,
                                   uint32_t subnetMask) {
  if (!pogHomeCandidateCompatible(candidate)) return false;
  if (!pogHomeCandidateCompatible(current)) return true;

  const uint8_t candidateRank =
      pogHomeCandidateRank(candidate, localIpv4, subnetMask);
  const uint8_t currentRank =
      pogHomeCandidateRank(current, localIpv4, subnetMask);
  if (candidateRank != currentRank) return candidateRank > currentRank;
  if (candidate.ipv4 != current.ipv4) return candidate.ipv4 < current.ipv4;
  return candidate.apiPort < current.apiPort;
}

}  // namespace pogdev
