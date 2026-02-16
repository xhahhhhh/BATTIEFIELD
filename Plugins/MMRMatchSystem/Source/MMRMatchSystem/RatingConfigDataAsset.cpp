#include "RatingConfigDataAsset.h"

FRankTier URatingConfigDataAsset::GetTierForRating(float Rating) const
{
	FRankTier Result;
	
	//先获取最低段位
	if (RankTiers.Num() >0)
	{
		Result = RankTiers[0];
	}
	
	for (const FRankTier& Tier : RankTiers)
	{
		if (Rating>=Tier.MinRating && Rating<Tier.MaxRating)
		{
			Result = Tier;
			break;
		}
	}
	return Result;
}
