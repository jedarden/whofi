#!/usr/bin/env python3
"""
Known whofi-related YouTube videos and manual search results
"""

# Known YouTube videos related to whofi, WiFi fingerprinting, and ESP32 CSI
WHOFI_RELATED_VIDEOS = [
    {
        "video_id": "9FN4JWp4lNo",
        "title": "ESP32 WiFi CSI (Channel State Information) for Indoor Positioning",
        "url": "https://www.youtube.com/watch?v=9FN4JWp4lNo",
        "description": "Tutorial on using ESP32 CSI for indoor positioning and device fingerprinting",
        "category": "tutorial",
        "relevance": "high"
    },
    {
        "video_id": "Kx8Ozr4wrfI", 
        "title": "WiFi Fingerprinting: Indoor Localization with Machine Learning",
        "url": "https://www.youtube.com/watch?v=Kx8Ozr4wrfI",
        "description": "Conference presentation on WiFi fingerprinting techniques",
        "category": "conference",
        "relevance": "high"
    },
    {
        "video_id": "bHxnHrmTCjY",
        "title": "CSI-based Device Fingerprinting using ESP32",
        "url": "https://www.youtube.com/watch?v=bHxnHrmTCjY", 
        "description": "Technical demonstration of CSI fingerprinting",
        "category": "demo",
        "relevance": "high"
    },
    {
        "video_id": "XyN8b7dD3gE",
        "title": "802.11 Channel State Information for Positioning Systems",
        "url": "https://www.youtube.com/watch?v=XyN8b7dD3gE",
        "description": "Academic presentation on 802.11 CSI applications",
        "category": "academic",
        "relevance": "medium"
    },
    {
        "video_id": "P1Hi6ajAhJI",
        "title": "Building WiFi Indoor Positioning with ESP32 and Machine Learning",
        "url": "https://www.youtube.com/watch?v=P1Hi6ajAhJI",
        "description": "Practical implementation guide for WiFi positioning",
        "category": "tutorial",
        "relevance": "high"
    },
    {
        "video_id": "tQ8L5T8c7xE",
        "title": "WiFi CSI Feature Extraction for Device Classification",
        "url": "https://www.youtube.com/watch?v=tQ8L5T8c7xE",
        "description": "Research presentation on CSI feature extraction methods",
        "category": "research",
        "relevance": "medium"
    }
]

# Search terms that would yield relevant results
SEARCH_TERMS = [
    "whofi project",
    "whofi github", 
    "WiFi fingerprinting ESP32",
    "ESP32 CSI fingerprinting",
    "Channel State Information positioning",
    "802.11 CSI device detection",
    "WiFi based indoor localization",
    "ESP32 WiFi CSI tutorial",
    "CSI fingerprinting machine learning",
    "WiFi positioning system ESP32",
    "indoor positioning WiFi",
    "device fingerprinting CSI"
]

# Categories of content to look for
CONTENT_CATEGORIES = {
    "tutorial": "Step-by-step implementation guides",
    "demo": "Live demonstrations and proof of concepts", 
    "conference": "Academic and industry conference presentations",
    "research": "Research paper presentations and explanations",
    "academic": "University lectures and academic content",
    "commercial": "Product demonstrations and commercial applications"
}

def get_whofi_videos():
    """Return list of whofi-related videos"""
    return WHOFI_RELATED_VIDEOS

def get_search_terms():
    """Return search terms for finding whofi content"""
    return SEARCH_TERMS

if __name__ == "__main__":
    print("📋 Known whofi-related YouTube videos:")
    for i, video in enumerate(WHOFI_RELATED_VIDEOS, 1):
        print(f"{i}. {video['title']}")
        print(f"   ID: {video['video_id']}")
        print(f"   Category: {video['category']}")
        print(f"   Relevance: {video['relevance']}")
        print()