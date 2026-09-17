#include "algorithms/MergeSort.hpp"

namespace dhaka
{

    void MergeSort::sortByUrgency(std::vector<CollectionPoint *> &points, bool descending)
    {
        if (points.size() <= 1)
            return;
        std::vector<CollectionPoint *> temp(points.size());
        mergeSortPointsInternal(points, temp, 0, static_cast<int>(points.size()) - 1, descending);
    }

    void MergeSort::mergePoints(std::vector<CollectionPoint *> &arr,
                                std::vector<CollectionPoint *> &temp,
                                int left, int mid, int right, bool descending)
    {
        int i = left;
        int j = mid + 1;
        int k = left;

        while (i <= mid && j <= right)
        {
            bool condition = descending ? (arr[i]->urgencyScore >= arr[j]->urgencyScore) : (arr[i]->urgencyScore <= arr[j]->urgencyScore);

            if (condition)
            {
                temp[k++] = arr[i++];
            }
            else
            {
                temp[k++] = arr[j++];
            }
        }

        while (i <= mid)
            temp[k++] = arr[i++];
        while (j <= right)
            temp[k++] = arr[j++];

        for (int p = left; p <= right; ++p)
        {
            arr[p] = temp[p];
        }
    }

    void MergeSort::mergeSortPointsInternal(std::vector<CollectionPoint *> &arr,
                                            std::vector<CollectionPoint *> &temp,
                                            int left, int right, bool descending)
    {
        if (left >= right)
            return;
        int mid = left + (right - left) / 2;
        mergeSortPointsInternal(arr, temp, left, mid, descending);
        mergeSortPointsInternal(arr, temp, mid + 1, right, descending);
        mergePoints(arr, temp, left, mid, right, descending);
    }

} // namespace dhaka
