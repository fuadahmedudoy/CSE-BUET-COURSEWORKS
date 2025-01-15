#include "tcp-vegas-a.h"

#include "tcp-socket-state.h"


#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpVegasA");
NS_OBJECT_ENSURE_REGISTERED(TcpVegasA);

TypeId
TcpVegasA::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::TcpVegasA")
                            .SetParent<TcpNewReno>()
                            .SetGroupName("Internet")
                            .AddConstructor<TcpVegasA>()
                            .AddAttribute("Alpha",
                                          "Lower threshold for Vegas congestion control",
                                          UintegerValue(2),
                                          MakeUintegerAccessor(&TcpVegasA::m_alpha),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("Beta",
                                          "Upper threshold for Vegas congestion control",
                                          UintegerValue(4),
                                          MakeUintegerAccessor(&TcpVegasA::m_beta),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("Gamma",
                                          "Limit on increase",
                                          UintegerValue(1),
                                          MakeUintegerAccessor(&TcpVegasA::m_gamma),
                                          MakeUintegerChecker<uint32_t>());

    return tid;
}

TcpVegasA::TcpVegasA()
      : TcpNewReno(),
      m_alpha(2),
      m_beta(4),
      m_gamma(1),
      m_baseRtt(Time::Max()),
      m_minRtt(Time::Max()),
      m_vegasEnabled(false),
      m_cntRtt(0),
      m_begSndNxt(0),
      m_currentThroughput(0.0),  // Th(t): Current throughput
      m_previousThroughput(0.0), // Th(t-rtt): Throughput one RTT ago
      m_rttBytesAcked(0.0),   // Bytes ACKed in the current RTT
      m_lastRtt(0)
{
    NS_LOG_FUNCTION(this);
}

TcpVegasA::TcpVegasA(const TcpVegasA& other)
    : TcpNewReno(other),
      m_alpha(other.m_alpha),
      m_beta(other.m_beta),
      m_gamma(other.m_gamma),
      m_baseRtt(other.m_baseRtt),
      m_minRtt(other.m_minRtt),
      m_cntRtt(other.m_cntRtt),
      m_vegasEnabled(other.m_vegasEnabled),
      m_begSndNxt(other.m_begSndNxt),
      m_currentThroughput(other.m_currentThroughput),  // Th(t): Current throughput
      m_previousThroughput(other.m_previousThroughput), // Th(t-rtt): Throughput one RTT ago
      m_rttBytesAcked(other.m_rttBytesAcked),   // Bytes ACKed in the current RTT
      m_lastRtt(other.m_lastRtt)
{
    NS_LOG_FUNCTION(this);
}

TcpVegasA::~TcpVegasA()
{
}

Ptr<TcpCongestionOps>
TcpVegasA::Fork()
{
    return CopyObject<TcpVegasA>(this);
}

void
TcpVegasA::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);

    if (rtt.IsZero())
    {
        return; // Invalid RTT, ignore
    }

    m_minRtt = std::min(m_minRtt, rtt);
    NS_LOG_DEBUG("Updated m_minRtt = " << m_minRtt);

    m_baseRtt = std::min(m_baseRtt, rtt);
    NS_LOG_DEBUG("Updated m_baseRtt = " << m_baseRtt);

    // Update RTT counter
    m_cntRtt++;
    NS_LOG_DEBUG("Updated m_cntRtt = " << m_cntRtt);
}

void
TcpVegasA::EnableVegas(Ptr<TcpSocketState> tcb)
{
    //NS_LOG_FUNCTION(this << tcb);

    m_vegasEnabled = true;
    m_begSndNxt = tcb->m_nextTxSequence;
    m_cntRtt = 0;
    m_minRtt = Time::Max();
}

void
TcpVegasA::DisableVegas()
{
    //NS_LOG_FUNCTION(this);

    m_vegasEnabled = false;
}

void
TcpVegasA::CongestionStateSet(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_FUNCTION(this << tcb << newState);
    if (newState == TcpSocketState::CA_OPEN)
    {
        EnableVegas(tcb);
    }
    else
    {
        DisableVegas();
    }
}

void
TcpVegasA::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);


     m_rttBytesAcked += segmentsAcked * tcb->m_segmentSize;

  // Fetch current RTT estimate
    Time currentRtt = tcb->m_srtt;//->GetCurrentEstimate ();

    if (currentRtt != m_lastRtt) // New RTT cycle detected
    {
      // Save the current throughput as Th(t-rtt)
        m_previousThroughput = m_currentThroughput;

        // Calculate Th(t): Current throughput over this RTT
        m_currentThroughput = (m_rttBytesAcked * 8.0) / currentRtt.GetSeconds (); // bits/sec
        NS_LOG_INFO ("Th(t): " << m_currentThroughput << " bps");
        NS_LOG_INFO ("Th(t-rtt): " << m_previousThroughput << " bps");

        // Reset for the next RTT cycle
        m_rttBytesAcked = 0;
        m_lastRtt = currentRtt;
    }

    if (!m_vegasEnabled)
    {
        NS_LOG_LOGIC("Vegas is not turned on, we follow NewReno algorithm.");
        TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
        return;
    }

    if (tcb->m_lastAckedSeq >= m_begSndNxt)
    {
        m_begSndNxt = tcb->m_nextTxSequence;

        if (m_cntRtt <= 2)
        {
            NS_LOG_LOGIC("Insufficient RTT samples; falling back to NewReno.");
            TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
        }
        else
        {
            uint32_t diff, targetCwnd, segCwnd = tcb->GetCwndInSegments();
            double tmp = m_baseRtt.GetSeconds() / m_minRtt.GetSeconds();
            targetCwnd = static_cast<uint32_t>(segCwnd * tmp);
            diff = segCwnd - targetCwnd;

            // Throughput comparison (approximate throughput difference)
            bool throughputIncreased = (m_baseRtt < m_minRtt);

            if (diff < m_beta && diff > m_alpha) // Between α and β
            {
                if(m_currentThroughput>m_previousThroughput){
                    segCwnd++;
                    m_alpha++;
                    m_beta++;
                }
                else NS_LOG_LOGIC("No adjustment needed; diff is between alpha and beta.");
            }
            
            
            else if (diff < m_alpha) // Case: diff < α
            {
                if (m_alpha>1 && m_currentThroughput>m_previousThroughput)
                {
                    segCwnd++;

                }
                else if (m_alpha>1 && m_currentThroughput<m_previousThroughput){
                    segCwnd--;
                    m_alpha--;
                    m_beta--;
                }

                else if(m_alpha==1)
                {
                    segCwnd++;
                    // AdjustAlphaBeta(diff, throughputIncreased);
                }
            }
            else if (diff > m_beta) // Case: diff > β
            {
                if(m_alpha>1){
                    segCwnd--;
                    m_alpha--;
                    m_beta--;
                }
            }

            tcb->m_cWnd = segCwnd * tcb->m_segmentSize;
            tcb->m_ssThresh = GetSsThresh(tcb, 0);
            NS_LOG_DEBUG("Updated cwnd = " << tcb->m_cWnd << ", ssthresh = " << tcb->m_ssThresh);
        }

        m_cntRtt = 0;
        m_minRtt = Time::Max();
    }
    else if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        TcpNewReno::SlowStart(tcb, segmentsAcked);
    }
}

void
TcpVegasA::AdjustAlphaBeta(double diff, bool throughputIncreased)
{
    if (diff > m_beta)
    {
        if (m_alpha > 1) // Avoid negative thresholds
        {
            m_alpha -= 1;
            m_beta -= 1;
        }
    }
    else if (diff < m_alpha)
    {
        if (throughputIncreased)
        {
            m_alpha += 1;
            m_beta += 1;
        }
    }

    NS_LOG_INFO("Adjusted thresholds: alpha = " << m_alpha << ", beta = " << m_beta);
}


void
TcpVegasA::UpdateBaseRtt(const Time& rtt)
{
    if (m_baseRtt.IsZero() || rtt < m_baseRtt)
    {
        m_baseRtt = rtt; // Update base RTT
    }
}
std::string
TcpVegasA::GetName() const
{
    return "TcpVegasA";
}

uint32_t
TcpVegasA::GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << tcb << bytesInFlight);
    return std::max(std::min(tcb->m_ssThresh.Get(), tcb->m_cWnd.Get() - tcb->m_segmentSize),
                    2 * tcb->m_segmentSize);
}

} // namespace ns3
