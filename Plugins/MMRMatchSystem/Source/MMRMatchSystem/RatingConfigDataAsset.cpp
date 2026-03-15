/**
 * @file RatingConfigDataAsset.cpp
 * @brief 评分配置数据资产实现
 */

#include "RatingConfigDataAsset.h"

FRankTier URatingConfigDataAsset::GetTierForRating(float Rating) const
{
	FRankTier Result;
	
	// 默认返回最低段位
	if (RankTiers.Num() > 0)
	{
		Result = RankTiers[0];
	}
	
	// 查找评分所在的段位范围
	for (const FRankTier& Tier : RankTiers)
	{
		if (Rating >= Tier.MinRating && Rating < Tier.MaxRating)
		{
			Result = Tier;
			break;
		}
	}
	
	return Result;
}
