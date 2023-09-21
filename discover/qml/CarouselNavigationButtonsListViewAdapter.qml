/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick

/*
 * Helper component to integrate CarouselNavigationButtons with a ListView.
 */
CarouselNavigationButtons {
    id: root

    enum Policy {
        AtXBeginningEnd,
        CurrentIndex
    }

    required property ListView view

    property /*Policy*/ int policy: CarouselNavigationButtonsListViewAdapter.CurrentIndex

    atBeginning: {
        switch (policy) {
        case CarouselNavigationButtonsListViewAdapter.AtXBeginningEnd:
            return view.atXBeginning;
        case CarouselNavigationButtonsListViewAdapter.CurrentIndex:
            return view.currentIndex === 0;
        default:
            return false;
        }
    }

    atEnd: {
        switch (policy) {
        case CarouselNavigationButtonsListViewAdapter.AtXBeginningEnd:
            return view.atXEnd;
        case CarouselNavigationButtonsListViewAdapter.CurrentIndex:
            return view.currentIndex === view.count - 1;
        default:
            return false;
        }
    }

    onDecrementCurrentIndex: {
        view.decrementCurrentIndex();
    }
    onIncrementCurrentIndex: {
        view.incrementCurrentIndex();
    }
}
