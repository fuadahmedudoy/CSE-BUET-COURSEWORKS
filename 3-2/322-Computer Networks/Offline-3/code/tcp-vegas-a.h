/*
 * TcpVegasA - Modified TCP Vegas
 * Extends TcpReno and introduces dynamic alpha and beta thresholds.
 */

#ifndef TCP_VEGAS_A_H
#define TCP_VEGAS_A_H

#include "tcp-congestion-ops.h"

namespace ns3 {

/**
 * \brief TcpVegasA: A modified version of TCP Vegas with dynamic alpha and beta.
 */
class TcpVegasA : public TcpNewReno
{
public:
    static TypeId GetTypeId(void);

    TcpVegasA();
    TcpVegasA(const TcpVegasA& other);
    ~TcpVegasA() override;
    std::string GetName() const override;

    Ptr<TcpCongestionOps> Fork() override;
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

private:
    void UpdateBaseRtt(const Time& rtt);
    void EnableVegas(Ptr<TcpSocketState> tcb);
    void AdjustAlphaBeta(double diff, bool throughputIncreased);

    /**
     * \brief Stop taking Vegas samples
     */
    void DisableVegas();

private:
    uint32_t m_alpha;          //!< Lower threshold for congestion control
    uint32_t m_beta;           //!< Upper threshold for congestion control
    uint32_t m_gamma;
    Time m_baseRtt;            //!< Smallest RTT observed
    Time m_minRtt;             //!< RTT for current RTT cycle
    uint32_t m_cntRtt; 
    bool m_vegasEnabled;       //!< Flag to enable VegasA logic
    SequenceNumber32 m_begSndNxt; //!< Starting sequence number for RTT cycle
    double m_currentThroughput;  // Th(t): Current throughput
    double m_previousThroughput; // Th(t-rtt): Throughput one RTT ago
    uint32_t m_rttBytesAcked;    // Bytes ACKed in the current RTT
    Time m_lastRtt; 
    
};

} // namespace ns3

#endif // TCP_VEGAS_A_H
