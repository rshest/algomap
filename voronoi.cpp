//  Implements Stephen J. Fortune's sweeping line algorithm for building Voronoi diagram
#include <geom.h>
#include <assert.h>

#include <voronoi.h>

static const float EPSILON = 1e-7f;

//  finds the point where two sites' parabolas meet
bool computeBreakPoint(const Point& p0, const Point& p1, float yL, float& bpx)
{
    float A = p1.y - p0.y;
    if (fabs(A) < EPSILON)
    {
        //  parabolas' sites are on the horizontal line, so breakpoint is in the middle
        bpx = 0.5f*(p0.x + p1.x);
        return true;
    }
    float B = p1.y - yL;
    float C = p0.y - yL;
    float b = 2.0f*(C*p1.x - B*p0.x);
    float c = B*p0.x*p0.x - C*p1.x*p1.x - A*B*C;
    // solve the quadratic equation
    float D = b*b - 4.0f*A*c;
    if (D < 0.0f)
    {
        return false;
    }
    //  taking the "closer" root
    bpx = (-b + sqrtf(D))*0.5f/A;
    return true;
}
    
//  returns the "circle event" point, which is the bottom of the circle through 3 given points
bool circleEventPoint(const Point& p0, const Point& p1, const Point& p2, Point& xc)
{
    float A = p1.x - p0.x;
    float B = p1.y - p0.y;
    float C = p2.x - p0.x;
    float D = p2.y - p0.y;
    float E = A*(p0.x + p1.x) + B*(p0.y + p1.y);
    float F = C*(p0.x + p2.x) + D*(p0.y + p2.y);
    float G = 2.0f*(A*(p2.y - p1.y) - B*(p2.x - p1.x));
    if (fabs(G) < EPSILON)
    {
        // all three points are on the same line
        return false;
    }
    float x0 = (D*E - B*F)/G;
    float y0 = (A*F - C*E)/G;
    float r = sqrtf((x0 - p0.x)*(x0 - p0.x) + (y0 - p0.y)*(y0 - p0.y));
    
    xc.x = x0;
    xc.y = y0 - r;
    
    return true;
}

// binary searches for the site, corresponding to the given coordinate x on the beachfront 
int findArc(float x, float yL, const Voronoi::BeachLine& beachLine)
{
    size_t n = beachLine.size();
    size_t l = 0;
    size_t r = n - 1;
    float bpx;
    while (r > l)
    {
        size_t k = (l + r)/2;
        const Point& p0 = beachLine[k].pt;
        const Point& p1 = beachLine[k + 1].pt;
        bool hasBP = computeBreakPoint(p0, p1, yL, bpx);
        assert(hasBP);
        if (bpx < x) l = k + 1; else r = k;
    }
    return l;
}


void Voronoi::init(const Point* sites, int numSites, const Rect& bounds)
{
    m_numSites = numSites;
    m_pSites = sites;

    // beach line size can not grow bigger than 2n-1
    m_beachLine.reserve(2*m_numSites - 1); 

    //  add all sites as "site events" to the event queue
    for (size_t i = 0; i < m_numSites; i++)
    {
        FEvent ev;
        ev.pt = sites[i];
        ev.type = FEvent::Site;
        m_events.push(ev);
    }
}

bool Voronoi::step()
{
    if (m_events.empty())
    {
        return false;
    }

    FEvent ev = m_events.top();
    m_events.pop();
    if (ev.type == FEvent::Site)
    {
        //  handle the site event
        if (m_beachLine.size() != 0)
        {
            size_t k = findArc(ev.pt.x, ev.pt.y, m_beachLine);
            //  break the arc
            FArc newArc;
            newArc.pt = ev.pt;
            m_beachLine.insert(m_beachLine.begin() + k, newArc);
            m_beachLine.insert(m_beachLine.begin() + k, m_beachLine[k + 1]);

            //  check for the potential circle events
            size_t l = std::max(k - 1, (size_t)0);
            size_t r = std::min(k + 1, m_beachLine.size() - 3);
            Point xc;
            for (size_t i = l; i <= r; i++)
            {
                if (circleEventPoint(m_beachLine[i].pt, m_beachLine[i + 1].pt, 
                    m_beachLine[i + 2].pt, xc))
                {
                    FEvent cev;
                    cev.type = FEvent::Circle;
                    cev.pt = xc;
                    m_events.push(cev);
                }
            }
        }
        else
        {
            //  this is the very first point in the beach line
            FArc arc;
            arc.pt = ev.pt;
            m_beachLine.push_back(arc);
        }
    }
    else
    {
        //  handle circle event
    }

    return true;
}


void Voronoi::run()
{
    while (step());
}
