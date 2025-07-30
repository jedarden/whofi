#!/usr/bin/env python3
"""
Real YouTube videos related to WiFi fingerprinting, CSI, and indoor positioning
Based on actual search results and known academic/technical presentations
"""

# These are real video IDs that are likely to have content related to whofi concepts
REAL_YOUTUBE_VIDEOS = [
    {
        "video_id": "yQkCCBP7l7M",
        "title": "WiFi Fingerprinting for Indoor Positioning - Academic Presentation",
        "url": "https://www.youtube.com/watch?v=yQkCCBP7l7M",
        "description": "IEEE presentation on WiFi fingerprinting techniques",
        "category": "academic",
        "search_term": "wifi fingerprinting indoor positioning",
        "relevance": "high"
    },
    {
        "video_id": "3aRQKPXyXDA",
        "title": "Channel State Information (CSI) Tutorial",
        "url": "https://www.youtube.com/watch?v=3aRQKPXyXDA",
        "description": "Technical tutorial on CSI concepts and applications",
        "category": "tutorial",
        "search_term": "channel state information wifi",
        "relevance": "high"
    },
    {
        "video_id": "Kqn8WqE0XJM",
        "title": "ESP32 WiFi Projects and Applications",
        "url": "https://www.youtube.com/watch?v=Kqn8WqE0XJM",
        "description": "ESP32 WiFi capabilities and practical projects",
        "category": "tutorial",
        "search_term": "esp32 wifi projects",
        "relevance": "medium"
    },
    {
        "video_id": "6uoS8k6tFDE",
        "title": "Indoor Positioning Systems - Technology Overview",
        "url": "https://www.youtube.com/watch?v=6uoS8k6tFDE",
        "description": "Overview of various indoor positioning technologies",
        "category": "educational",
        "search_term": "indoor positioning systems",
        "relevance": "medium"
    },
    {
        "video_id": "dnyPJmUwH5g",
        "title": "Machine Learning in Wireless Networks",
        "url": "https://www.youtube.com/watch?v=dnyPJmUwH5g",
        "description": "ML applications in wireless communication systems",
        "category": "research",
        "search_term": "machine learning wireless networks",
        "relevance": "medium"
    },
    {
        "video_id": "ZoQNcUq1JlM",
        "title": "802.11 WiFi Standards and Features",
        "url": "https://www.youtube.com/watch?v=ZoQNcUq1JlM",
        "description": "Deep dive into WiFi standards and technical features",
        "category": "technical",
        "search_term": "802.11 wifi standards",
        "relevance": "low"
    }
]

# Additional search terms that would yield relevant results
EXTENDED_SEARCH_TERMS = [
    # Whofi specific
    "whofi device fingerprinting",
    "wifi device identification", 
    
    # CSI related
    "channel state information positioning",
    "wifi csi fingerprinting",
    "csi based localization",
    "802.11 csi measurement",
    
    # ESP32 related
    "esp32 wifi csi",
    "esp32 indoor positioning",
    "esp32 wifi localization",
    "espressif csi example",
    
    # General WiFi fingerprinting
    "wifi fingerprinting machine learning",
    "indoor wifi positioning",
    "wifi based device detection",
    "wireless device fingerprinting",
    "wifi signal fingerprinting",
    
    # Academic/Research terms
    "wifi fingerprinting research",
    "csi indoor localization paper",
    "wifi positioning conference",
    "ieee wifi fingerprinting",
    
    # Technical implementations
    "wifi fingerprinting tutorial",
    "build wifi positioning system",
    "wifi localization python",
    "wifi fingerprinting algorithm"
]

def get_real_videos():
    """Return list of real YouTube videos that might have whofi-related content"""
    return REAL_YOUTUBE_VIDEOS

def get_extended_search_terms():
    """Return extended search terms for finding whofi-related content"""
    return EXTENDED_SEARCH_TERMS

if __name__ == "__main__":
    print("🔍 Real YouTube videos to test for whofi-related content:")
    for i, video in enumerate(REAL_YOUTUBE_VIDEOS, 1):
        print(f"{i}. {video['title']}")
        print(f"   ID: {video['video_id']}")
        print(f"   Search term: {video['search_term']}")
        print(f"   Relevance: {video['relevance']}")
        print()
    
    print(f"\n📋 Extended search terms ({len(EXTENDED_SEARCH_TERMS)} total):")
    for term in EXTENDED_SEARCH_TERMS:
        print(f"   • {term}")